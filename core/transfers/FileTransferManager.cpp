#include "FileTransferManager.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QUuid>

#include "../crypto/CryptoUtils.hpp"

namespace peerchat::transfers {

QString transferStateToString(TransferState state) {
    switch (state) {
        case TransferState::Pending:
            return QStringLiteral("pending");
        case TransferState::Active:
            return QStringLiteral("active");
        case TransferState::Paused:
            return QStringLiteral("paused");
        case TransferState::Completed:
            return QStringLiteral("completed");
        case TransferState::Failed:
            return QStringLiteral("failed");
        case TransferState::Cancelled:
        default:
            return QStringLiteral("cancelled");
    }
}

TransferState stringToTransferState(const QString& str) {
    if (str == QLatin1String("pending"))
        return TransferState::Pending;
    if (str == QLatin1String("active"))
        return TransferState::Active;
    if (str == QLatin1String("paused"))
        return TransferState::Paused;
    if (str == QLatin1String("completed"))
        return TransferState::Completed;
    if (str == QLatin1String("failed"))
        return TransferState::Failed;
    return TransferState::Cancelled;
}

QString IFileTransfer::sanitizeFilename(const QString& rawName) {
    QString clean = rawName;
    // Normalize both Windows and Unix path separators
    clean.replace(QLatin1Char('\\'), QLatin1Char('/'));

    // Strip leading path components
    qsizetype lastSlash = clean.lastIndexOf(QLatin1Char('/'));
    if (lastSlash != -1) {
        clean = clean.mid(lastSlash + 1);
    }

    // Remove control characters (ASCII 0-31 and 127)
    static const QRegularExpression controlRegex(QStringLiteral("[\\x00-\\x1F\\x7F]"));
    clean.remove(controlRegex);

    clean = clean.trimmed();

    // Guard against relative traversal tokens, empty strings, and pure dot names
    if (clean.isEmpty() || clean == QLatin1String(".") || clean == QLatin1String("..") ||
        clean.startsWith(QLatin1String(".."))) {
        clean = QStringLiteral("attachment_") +
                QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }
    return clean;
}

FileTransferManager::FileTransferManager(networking::IPeerTransport* transport,
                                         storage::IMessageStore* store,
                                         identity::IIdentityProvider* identityProvider,
                                         const QString& downloadDir, QObject* parent)
    : IFileTransfer(parent),
      m_transport(transport),
      m_store(store),
      m_identityProvider(identityProvider),
      m_downloadDir(downloadDir) {
    QDir dir(m_downloadDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_outgoingChunkTimer = new QTimer(this);
    m_outgoingChunkTimer->setInterval(15);  // Stream chunks with low latency
    connect(m_outgoingChunkTimer, &QTimer::timeout, this, &FileTransferManager::sendNextChunks);
}

FileTransferManager::~FileTransferManager() {
    for (auto& out : m_outgoingTransfers) {
        if (out && out->file && out->file->isOpen())
            out->file->close();
    }
    for (auto& in : m_incomingTransfers) {
        if (in && in->partFile && in->partFile->isOpen())
            in->partFile->close();
    }
}

void FileTransferManager::setDownloadDirectory(const QString& dir) {
    m_downloadDir = dir;
    QDir d(m_downloadDir);
    if (!d.exists()) {
        d.mkpath(".");
    }
}

QString FileTransferManager::downloadDirectory() const {
    return m_downloadDir;
}

QString FileTransferManager::resolveUniqueFilePath(const QString& directory,
                                                   const QString& filename) {
    QDir dir(directory);
    QString fullPath = dir.filePath(filename);
    if (!QFile::exists(fullPath)) {
        return fullPath;
    }

    QFileInfo fi(filename);
    QString base = fi.completeBaseName();
    QString suffix = fi.suffix();
    if (!suffix.isEmpty())
        suffix = QStringLiteral(".") + suffix;

    int counter = 1;
    while (true) {
        QString candidate = QStringLiteral("%1 (%2)%3").arg(base).arg(counter).arg(suffix);
        fullPath = dir.filePath(candidate);
        if (!QFile::exists(fullPath)) {
            return fullPath;
        }
        counter++;
    }
}

QString FileTransferManager::startSendFile(const QString& peerId, const QString& localFilePath) {
    QFileInfo fi(localFilePath);
    if (!fi.exists() || !fi.isFile()) {
        return {};
    }

    QString transferId = QStringLiteral("tx_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    qint64 fileSize = fi.size();
    QString safeName = sanitizeFilename(fi.fileName());

    // Compute whole-file SHA-256 via streaming chunks (never load entire file into memory)
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QFile hashFile(localFilePath);
    if (!hashFile.open(QIODevice::ReadOnly)) {
        return {};
    }
    char buf[65536];
    while (!hashFile.atEnd()) {
        qint64 read = hashFile.read(buf, sizeof(buf));
        if (read > 0) {
            hash.addData(QByteArrayView(buf, static_cast<qsizetype>(read)));
        }
    }
    hashFile.close();
    QString sha256Hex = QString::fromUtf8(hash.result().toHex());

    QMimeDatabase mimeDb;
    QString mime = mimeDb.mimeTypeForFile(localFilePath).name();

    int totalChunks =
        (fileSize == 0) ? 1 : static_cast<int>((fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);

    TransferInfo info;
    info.transferId = transferId;
    info.peerId = peerId;
    info.fileName = safeName;
    info.filePath = localFilePath;
    info.fileSize = fileSize;
    info.sha256 = sha256Hex;
    info.transferredBytes = 0;
    info.state = TransferState::Pending;
    info.isOutgoing = true;
    info.totalChunks = totalChunks;
    info.completedChunks = 0;
    info.mimeType = mime;

    auto out = std::make_shared<ActiveOutgoing>();
    out->info = info;
    out->file = std::make_unique<QFile>(localFilePath);
    if (!out->file->open(QIODevice::ReadOnly)) {
        return {};
    }
    out->nextChunkIndex = 0;
    m_outgoingTransfers[transferId] = out;

    // Save to store
    if (m_store) {
        storage::StoredTransfer st;
        st.id = transferId;
        st.messageId = transferId;
        st.fileName = safeName;
        st.filePath = localFilePath;
        st.fileSize = fileSize;
        st.sha256 = sha256Hex;
        st.transferredBytes = 0;
        st.status = QStringLiteral("pending");
        st.direction = QStringLiteral("outgoing");
        st.timestamp = QDateTime::currentMSecsSinceEpoch();
        m_store->saveTransfer(st);
    }

    // Send file offer to peer
    if (m_transport && m_identityProvider) {
        messaging::ProtocolMessage msg;
        msg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        msg.type = messaging::MessageType::FileOffer;
        msg.senderId = m_identityProvider->peerId();
        msg.recipientId = peerId;
        msg.timestamp = QDateTime::currentMSecsSinceEpoch();

        QJsonObject p;
        p["transfer_id"] = transferId;
        p["filename"] = safeName;
        p["mime_type"] = mime;
        p["total_size"] = fileSize;
        p["sha256"] = sha256Hex;
        p["chunk_size"] = CHUNK_SIZE;
        p["total_chunks"] = totalChunks;
        msg.payload = p;

        msg.signature = m_identityProvider->sign(msg.canonicalPayloadBytes());
        m_transport->sendPacket(peerId, msg.toJson());
    }

    return transferId;
}

bool FileTransferManager::acceptFileOffer(const QString& transferId) {
    if (!m_pendingIncomingOffers.contains(transferId)) {
        return false;
    }

    TransferInfo info = m_pendingIncomingOffers.take(transferId);
    info.state = TransferState::Active;

    QString partPath = QDir(m_downloadDir).filePath(transferId + QStringLiteral(".part"));
    QString finalPath = resolveUniqueFilePath(m_downloadDir, info.fileName);

    auto in = std::make_shared<ActiveIncoming>();
    in->info = info;
    in->targetFinalPath = finalPath;
    in->partFile = std::make_unique<QFile>(partPath);

    if (!in->partFile->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        emit transferFailed(transferId,
                            QStringLiteral("Cannot open destination temporary file for writing"));
        return false;
    }

    m_incomingTransfers[transferId] = in;

    // Persist
    if (m_store) {
        storage::StoredTransfer st;
        st.id = transferId;
        st.messageId = transferId;
        st.fileName = info.fileName;
        st.filePath = finalPath;
        st.fileSize = info.fileSize;
        st.sha256 = info.sha256;
        st.transferredBytes = 0;
        st.status = QStringLiteral("active");
        st.direction = QStringLiteral("incoming");
        st.timestamp = QDateTime::currentMSecsSinceEpoch();
        m_store->saveTransfer(st);
    }

    // Send accept control message
    if (m_transport && m_identityProvider) {
        messaging::ProtocolMessage msg;
        msg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        msg.type = messaging::MessageType::FileControl;
        msg.senderId = m_identityProvider->peerId();
        msg.recipientId = info.peerId;
        msg.timestamp = QDateTime::currentMSecsSinceEpoch();

        QJsonObject p;
        p["transfer_id"] = transferId;
        p["action"] = QStringLiteral("accept");
        msg.payload = p;
        msg.signature = m_identityProvider->sign(msg.canonicalPayloadBytes());
        m_transport->sendPacket(info.peerId, msg.toJson());
    }

    // If zero byte file, complete immediately
    if (info.fileSize == 0) {
        finishIncomingTransfer(transferId);
    }

    return true;
}

void FileTransferManager::rejectFileOffer(const QString& transferId) {
    if (m_pendingIncomingOffers.contains(transferId)) {
        TransferInfo info = m_pendingIncomingOffers.take(transferId);
        if (m_transport && m_identityProvider) {
            messaging::ProtocolMessage msg;
            msg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            msg.type = messaging::MessageType::FileControl;
            msg.senderId = m_identityProvider->peerId();
            msg.recipientId = info.peerId;
            msg.timestamp = QDateTime::currentMSecsSinceEpoch();

            QJsonObject p;
            p["transfer_id"] = transferId;
            p["action"] = QStringLiteral("reject");
            msg.payload = p;
            msg.signature = m_identityProvider->sign(msg.canonicalPayloadBytes());
            m_transport->sendPacket(info.peerId, msg.toJson());
        }
        emit transferCancelled(transferId);
    }
}

void FileTransferManager::cancelTransfer(const QString& transferId) {
    QString peerId;
    if (m_outgoingTransfers.contains(transferId)) {
        auto out = m_outgoingTransfers.take(transferId);
        peerId = out->info.peerId;
        if (out->file && out->file->isOpen())
            out->file->close();
    } else if (m_incomingTransfers.contains(transferId)) {
        auto in = m_incomingTransfers.take(transferId);
        peerId = in->info.peerId;
        if (in->partFile && in->partFile->isOpen()) {
            in->partFile->close();
            in->partFile->remove();
        }
    } else if (m_pendingIncomingOffers.contains(transferId)) {
        rejectFileOffer(transferId);
        return;
    }

    if (!peerId.isEmpty() && m_transport && m_identityProvider) {
        messaging::ProtocolMessage msg;
        msg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        msg.type = messaging::MessageType::FileControl;
        msg.senderId = m_identityProvider->peerId();
        msg.recipientId = peerId;
        msg.timestamp = QDateTime::currentMSecsSinceEpoch();

        QJsonObject p;
        p["transfer_id"] = transferId;
        p["action"] = QStringLiteral("cancel");
        msg.payload = p;
        msg.signature = m_identityProvider->sign(msg.canonicalPayloadBytes());
        m_transport->sendPacket(peerId, msg.toJson());
    }

    if (m_store) {
        m_store->updateTransferProgress(transferId, 0, QStringLiteral("cancelled"));
    }
    emit transferCancelled(transferId);
}

void FileTransferManager::pauseTransfer(const QString& transferId) {
    if (m_outgoingTransfers.contains(transferId)) {
        m_outgoingTransfers[transferId]->isPaused = true;
    } else if (m_incomingTransfers.contains(transferId)) {
        m_incomingTransfers[transferId]->isPaused = true;
    }
}

void FileTransferManager::resumeTransfer(const QString& transferId) {
    if (m_outgoingTransfers.contains(transferId)) {
        m_outgoingTransfers[transferId]->isPaused = false;
        if (!m_outgoingChunkTimer->isActive()) {
            m_outgoingChunkTimer->start();
        }
    } else if (m_incomingTransfers.contains(transferId)) {
        m_incomingTransfers[transferId]->isPaused = false;
    }
}

void FileTransferManager::sendNextChunks() {
    bool hasActive = false;

    for (auto it = m_outgoingTransfers.begin(); it != m_outgoingTransfers.end(); ++it) {
        auto out = it.value();
        if (!out || out->isPaused || out->info.state != TransferState::Active) {
            continue;
        }

        hasActive = true;
        if (out->nextChunkIndex >= out->info.totalChunks) {
            // Done sending all chunks
            if (out->file && out->file->isOpen())
                out->file->close();
            out->info.state = TransferState::Completed;
            emit transferCompleted(out->info.transferId, out->info.filePath);
            if (m_store) {
                m_store->updateTransferProgress(out->info.transferId, out->info.fileSize,
                                                QStringLiteral("completed"));
            }
            continue;
        }

        // Read one chunk from file
        qint64 offset = static_cast<qint64>(out->nextChunkIndex) * CHUNK_SIZE;
        out->file->seek(offset);
        QByteArray chunkBytes = out->file->read(CHUNK_SIZE);

        messaging::ProtocolMessage chunkMsg;
        chunkMsg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        chunkMsg.type = messaging::MessageType::FileChunk;
        chunkMsg.senderId = m_identityProvider->peerId();
        chunkMsg.recipientId = out->info.peerId;
        chunkMsg.timestamp = QDateTime::currentMSecsSinceEpoch();

        QJsonObject p;
        p["transfer_id"] = out->info.transferId;
        p["chunk_index"] = out->nextChunkIndex;
        p["chunk_data"] = QString::fromUtf8(chunkBytes.toBase64());
        chunkMsg.payload = p;
        chunkMsg.signature = m_identityProvider->sign(chunkMsg.canonicalPayloadBytes());

        m_transport->sendPacket(out->info.peerId, chunkMsg.toJson());

        out->nextChunkIndex++;
        out->info.transferredBytes += chunkBytes.size();
        emit transferProgress(out->info.transferId, out->info.transferredBytes, out->info.fileSize);

        if (m_store) {
            m_store->updateTransferProgress(out->info.transferId, out->info.transferredBytes,
                                            QStringLiteral("active"));
        }
    }

    if (!hasActive && m_outgoingChunkTimer->isActive()) {
        m_outgoingChunkTimer->stop();
    }
}

void FileTransferManager::handleProtocolMessage(const messaging::ProtocolMessage& msg) {
    switch (msg.type) {
        case messaging::MessageType::FileOffer: {
            QString transferId = msg.payload.value("transfer_id").toString();
            QString rawFilename = msg.payload.value("filename").toString();
            QString safeFilename = sanitizeFilename(rawFilename);
            qint64 totalSize = msg.payload.value("total_size").toInteger();
            QString sha256 = msg.payload.value("sha256").toString();
            int totalChunks = msg.payload.value("total_chunks").toInt();
            QString mime = msg.payload.value("mime_type").toString();

            TransferInfo info;
            info.transferId = transferId;
            info.peerId = msg.senderId;
            info.fileName = safeFilename;
            info.fileSize = totalSize;
            info.sha256 = sha256;
            info.transferredBytes = 0;
            info.state = TransferState::Pending;
            info.isOutgoing = false;
            info.totalChunks = totalChunks;
            info.completedChunks = 0;
            info.mimeType = mime;

            m_pendingIncomingOffers[transferId] = info;
            emit transferOffered(info);
            break;
        }

        case messaging::MessageType::FileControl: {
            QString transferId = msg.payload.value("transfer_id").toString();
            QString action = msg.payload.value("action").toString();

            if (action == QLatin1String("accept")) {
                if (m_outgoingTransfers.contains(transferId)) {
                    m_outgoingTransfers[transferId]->info.state = TransferState::Active;
                    if (!m_outgoingChunkTimer->isActive()) {
                        m_outgoingChunkTimer->start();
                    }
                }
            } else if (action == QLatin1String("reject") || action == QLatin1String("cancel")) {
                cancelTransfer(transferId);
            }
            break;
        }

        case messaging::MessageType::FileChunk: {
            QString transferId = msg.payload.value("transfer_id").toString();
            if (!m_incomingTransfers.contains(transferId)) {
                return;
            }

            auto in = m_incomingTransfers[transferId];
            if (in->isPaused)
                return;

            int chunkIndex = msg.payload.value("chunk_index").toInt();
            QByteArray chunkBytes =
                QByteArray::fromBase64(msg.payload.value("chunk_data").toString().toUtf8());

            qint64 offset = static_cast<qint64>(chunkIndex) * CHUNK_SIZE;
            in->partFile->seek(offset);
            in->partFile->write(chunkBytes);
            in->partFile->flush();

            in->info.transferredBytes += chunkBytes.size();
            in->info.completedChunks++;

            emit transferProgress(transferId, in->info.transferredBytes, in->info.fileSize);
            if (m_store) {
                m_store->updateTransferProgress(transferId, in->info.transferredBytes,
                                                QStringLiteral("active"));
            }

            // Check if transfer complete
            if (in->info.transferredBytes >= in->info.fileSize) {
                finishIncomingTransfer(transferId);
            }
            break;
        }

        default:
            break;
    }
}

void FileTransferManager::finishIncomingTransfer(const QString& transferId) {
    if (!m_incomingTransfers.contains(transferId))
        return;

    auto in = m_incomingTransfers.take(transferId);
    in->partFile->close();

    // Verify cryptographic SHA-256 integrity
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QFile verifyFile(in->partFile->fileName());
    if (verifyFile.open(QIODevice::ReadOnly)) {
        char buf[65536];
        while (!verifyFile.atEnd()) {
            qint64 read = verifyFile.read(buf, sizeof(buf));
            if (read > 0)
                hash.addData(QByteArrayView(buf, static_cast<qsizetype>(read)));
        }
        verifyFile.close();
    }

    QString actualSha256 = QString::fromUtf8(hash.result().toHex());
    if (in->info.fileSize > 0 && !in->info.sha256.isEmpty() &&
        actualSha256.compare(in->info.sha256, Qt::CaseInsensitive) != 0) {
        // Integrity mismatch! Delete corrupted .part file
        QFile::remove(in->partFile->fileName());
        if (m_store) {
            m_store->updateTransferProgress(transferId, 0, QStringLiteral("failed"));
        }
        emit transferFailed(transferId,
                            QStringLiteral("Integrity verification failed: SHA-256 mismatch"));
        return;
    }

    // Atomic promote .part file to target final destination
    if (QFile::exists(in->targetFinalPath)) {
        QFile::remove(in->targetFinalPath);
    }
    bool renameOk = QFile::rename(in->partFile->fileName(), in->targetFinalPath);
    if (!renameOk) {
        // Fallback copy & delete
        QFile::copy(in->partFile->fileName(), in->targetFinalPath);
        QFile::remove(in->partFile->fileName());
    }

    if (m_store) {
        m_store->updateTransferProgress(transferId, in->info.fileSize, QStringLiteral("completed"));
    }
    emit transferCompleted(transferId, in->targetFinalPath);
}

}  // namespace peerchat::transfers
