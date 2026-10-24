#include "ChatController.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace peerchat::client {

static QString formatBytes(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(static_cast<double>(bytes) / 1024.0, 'f', 1) + " KB";
    return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1) + " MB";
}

ChatController::ChatController(QObject* parent)
    : QObject(parent),
      m_contactModel(new ContactModel(this)),
      m_messageModel(new MessageModel(this)),
      m_transferModel(new TransferModel(this)) {
}

ChatController::~ChatController() = default;

bool ChatController::initialize(const QString& profile, quint16 preferredPort, const QString& customStoragePath) {
    QString storageDir = customStoragePath;
    if (storageDir.isEmpty()) {
        QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (base.isEmpty()) base = QDir::homePath() + "/.peerchat";
        if (!profile.isEmpty()) {
            base += "/" + profile;
        }
        storageDir = base;
    }

    QDir d(storageDir);
    if (!d.exists()) d.mkpath(".");

    m_identity = new identity::SodiumIdentityProvider(this);
    m_store = new storage::SqliteMessageStore(this);
    m_directTransport = new networking::DirectTcpTransport(m_identity, this);
    m_relayTransport = new networking::RelayTransport(m_identity, this);

    QString downloadsDir = storageDir + "/downloads";
    m_transferMgr = new transfers::FileTransferManager(m_directTransport, m_store, m_identity, downloadsDir, this);
    m_discovery = new discovery::LanDiscovery(m_identity, preferredPort, this);
    m_diagnostics = new diagnostics::DiagnosticsService(this);

    m_engine = new messaging::MessagingEngine(m_identity, m_directTransport, m_relayTransport,
                                              m_discovery, m_store, m_transferMgr, this);

    connect(m_engine, &messaging::MessagingEngine::messageReceived, this, &ChatController::onMessageReceived);
    connect(m_engine, &messaging::MessagingEngine::messageDelivered, this, &ChatController::onMessageDelivered);
    connect(m_engine, &messaging::MessagingEngine::peerRouteChanged, this, &ChatController::onPeerRouteChanged);
    connect(m_engine, &messaging::MessagingEngine::contactDiscovered, this, &ChatController::onContactDiscovered);

    connect(m_transferMgr, &transfers::IFileTransfer::transferOffered, this, [this](const transfers::TransferInfo& info) {
        TransferListItem item;
        item.transferId = info.transferId;
        item.fileName = info.fileName;
        item.fileSize = info.fileSize;
        item.progress = 0.0;
        item.status = "offered";
        item.isOutgoing = false;
        item.sizeStr = formatBytes(info.fileSize);
        m_transferModel->updateTransfer(item);
        emit newNotification("Incoming File", info.fileName + " (" + item.sizeStr + ")");
    });

    connect(m_transferMgr, &transfers::IFileTransfer::transferProgress, this, [this](const QString& id, qint64 done, qint64 total) {
        TransferListItem item;
        item.transferId = id;
        item.transferredBytes = done;
        item.fileSize = total;
        item.progress = total > 0 ? static_cast<double>(done) / static_cast<double>(total) : 0.0;
        item.status = "transferring";
        item.sizeStr = formatBytes(done) + " / " + formatBytes(total);
        m_transferModel->updateTransfer(item);
    });

    connect(m_transferMgr, &transfers::IFileTransfer::transferCompleted, this, [this](const QString& id, const QString& path) {
        TransferListItem item;
        item.transferId = id;
        item.filePath = path;
        item.progress = 1.0;
        item.status = "completed";
        m_transferModel->updateTransfer(item);
        emit newNotification("Transfer Completed", QFileInfo(path).fileName());
    });

    bool ok = m_engine->initialize(storageDir, profile);
    if (!ok) return false;

    if (preferredPort > 0) {
        m_directTransport->startListening(preferredPort);
        m_discovery->setTcpListenPort(m_directTransport->listeningPort());
    }

    emit myPeerIdChanged();
    emit myDisplayNameChanged();
    emit listenPortChanged();

    // Populate contacts
    QList<storage::Contact> saved = m_store->getAllContacts();
    for (const auto& c : saved) {
        ContactItem ci;
        ci.peerId = c.peerId;
        ci.displayName = c.displayName;
        ci.routeType = networking::routeTypeToString(m_engine->getRoute(c.peerId));
        ci.lastSeen = c.lastSeen;
        m_contactModel->updateOrAddContact(ci);
    }

    return true;
}

QString ChatController::myPeerId() const {
    return m_identity ? m_identity->peerId() : QString();
}

QString ChatController::myDisplayName() const {
    return m_identity ? m_identity->displayName() : QString();
}

void ChatController::setMyDisplayName(const QString& name) {
    if (m_identity && !name.trimmed().isEmpty()) {
        m_identity->setDisplayName(name);
        emit myDisplayNameChanged();
    }
}

QString ChatController::activeContactPeerId() const {
    return m_activeContactPeerId;
}

QString ChatController::activeContactName() const {
    return m_activeContactName;
}

QString ChatController::activeRouteType() const {
    if (m_activeContactPeerId.isEmpty() || !m_engine) return QStringLiteral("Offline");
    return networking::routeTypeToString(m_engine->getRoute(m_activeContactPeerId));
}

quint16 ChatController::listenPort() const {
    return m_directTransport ? m_directTransport->listeningPort() : 0;
}

void ChatController::setDarkTheme(bool dark) {
    if (m_darkTheme != dark) {
        m_darkTheme = dark;
        emit themeChanged();
    }
}

void ChatController::toggleTheme() {
    setDarkTheme(!m_darkTheme);
}

void ChatController::selectContact(const QString& peerId) {
    m_activeContactPeerId = peerId;
    auto opt = m_store ? m_store->getContact(peerId) : std::nullopt;
    m_activeContactName = opt.has_value() ? opt->displayName : peerId.left(12);

    emit activeContactChanged();
    emit activeRouteChanged();

    // Load conversation messages
    if (m_store && m_identity) {
        QString myId = m_identity->peerId();
        QString convId = myId < peerId ? myId + "_" + peerId : peerId + "_" + myId;
        QList<storage::StoredMessage> msgs = m_store->getMessagesForConversation(convId);

        QList<MessageItem> items;
        for (const auto& m : msgs) {
            MessageItem mi;
            mi.id = m.id;
            mi.senderId = m.senderId;
            mi.content = m.content;
            mi.mediaPath = m.mediaPath;
            mi.statusStr = storage::messageStatusToString(m.status);
            mi.isOutgoing = (m.senderId == myId);
            mi.timestamp = m.timestamp;
            mi.timeFormatted = QDateTime::fromMSecsSinceEpoch(m.timestamp).toString("hh:mm");
            items.append(mi);
        }
        m_messageModel->setMessages(items);
    }
}

void ChatController::sendMessage(const QString& text) {
    if (m_activeContactPeerId.isEmpty() || text.trimmed().isEmpty() || !m_engine) {
        return;
    }

    QString msgId = m_engine->sendTextMessage(m_activeContactPeerId, text.trimmed());

    MessageItem mi;
    mi.id = msgId;
    mi.senderId = myPeerId();
    mi.content = text.trimmed();
    mi.statusStr = QStringLiteral("sent");
    mi.isOutgoing = true;
    mi.timestamp = QDateTime::currentMSecsSinceEpoch();
    mi.timeFormatted = QDateTime::currentDateTime().toString("hh:mm");

    m_messageModel->appendMessage(mi);
}

void ChatController::sendFile(const QString& filePath) {
    if (m_activeContactPeerId.isEmpty() || filePath.isEmpty() || !m_engine) {
        return;
    }

    // Strip file:// prefix if present from QML FileDialog
    QString cleanPath = filePath;
    if (cleanPath.startsWith("file://")) {
        cleanPath = cleanPath.mid(7);
    }

    QString txId = m_engine->sendFile(m_activeContactPeerId, cleanPath);
    if (!txId.isEmpty()) {
        QFileInfo fi(cleanPath);
        TransferListItem item;
        item.transferId = txId;
        item.fileName = fi.fileName();
        item.filePath = cleanPath;
        item.fileSize = fi.size();
        item.progress = 0.0;
        item.status = "sending_offer";
        item.isOutgoing = true;
        item.sizeStr = formatBytes(fi.size());
        m_transferModel->updateTransfer(item);
    }
}

void ChatController::addContact(const QString& peerId, const QString& displayName) {
    if (peerId.trimmed().isEmpty()) return;

    QString cleanPeerId = peerId.trimmed();
    QString name = displayName.trimmed().isEmpty() ? cleanPeerId.left(12) : displayName.trimmed();

    if (m_engine) {
        m_engine->addOrUpdateContact(cleanPeerId, name);
    }

    ContactItem ci;
    ci.peerId = cleanPeerId;
    ci.displayName = name;
    ci.routeType = "Offline";
    ci.lastSeen = QDateTime::currentMSecsSinceEpoch();
    m_contactModel->updateOrAddContact(ci);
}

void ChatController::connectDirect(const QString& peerId, const QString& host, quint16 port) {
    if (m_engine) {
        m_engine->connectToPeerDirect(peerId, host, port);
    }
}

void ChatController::acceptTransfer(const QString& transferId) {
    if (m_transferMgr) {
        m_transferMgr->acceptFileOffer(transferId);
    }
}

void ChatController::cancelTransfer(const QString& transferId) {
    if (m_transferMgr) {
        m_transferMgr->cancelTransfer(transferId);
    }
}

void ChatController::configureRelay(const QString& host, quint16 port, const QString& token) {
    if (m_relayTransport) {
        m_relayTransport->setRelayServer(host, port, token);
    }
}

void ChatController::onMessageReceived(const storage::StoredMessage& msg) {
    if (msg.senderId == m_activeContactPeerId) {
        MessageItem mi;
        mi.id = msg.id;
        mi.senderId = msg.senderId;
        mi.content = msg.content;
        mi.mediaPath = msg.mediaPath;
        mi.statusStr = QStringLiteral("delivered");
        mi.isOutgoing = false;
        mi.timestamp = msg.timestamp;
        mi.timeFormatted = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("hh:mm");
        m_messageModel->appendMessage(mi);
    }

    ContactItem ci;
    ci.peerId = msg.senderId;
    auto opt = m_store ? m_store->getContact(msg.senderId) : std::nullopt;
    ci.displayName = opt.has_value() ? opt->displayName : msg.senderId.left(12);
    ci.lastSeen = msg.timestamp;
    ci.lastMessage = msg.content;
    ci.routeType = networking::routeTypeToString(m_engine->getRoute(msg.senderId));
    m_contactModel->updateOrAddContact(ci);

    emit newNotification(ci.displayName, msg.content);
}

void ChatController::onMessageDelivered(const QString& msgId, qint64 deliveredAt) {
    Q_UNUSED(deliveredAt);
    m_messageModel->updateStatus(msgId, QStringLiteral("delivered"));
}

void ChatController::onPeerRouteChanged(const QString& peerId, const QString& routeString) {
    m_contactModel->updateRoute(peerId, routeString);
    if (peerId == m_activeContactPeerId) {
        emit activeRouteChanged();
    }
}

void ChatController::onContactDiscovered(const QString& peerId, const QString& displayName, const QString& host, quint16 port) {
    Q_UNUSED(host);
    Q_UNUSED(port);
    ContactItem ci;
    ci.peerId = peerId;
    ci.displayName = displayName;
    ci.routeType = networking::routeTypeToString(m_engine->getRoute(peerId));
    ci.lastSeen = QDateTime::currentMSecsSinceEpoch();
    m_contactModel->updateOrAddContact(ci);
}

}  // namespace peerchat::client
