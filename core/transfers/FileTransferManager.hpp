#pragma once

#include "IFileTransfer.hpp"
#include "../networking/IPeerTransport.hpp"
#include "../storage/IMessageStore.hpp"
#include "../identity/IIdentityProvider.hpp"
#include "../messaging/ProtocolMessage.hpp"

#include <QFile>
#include <QMap>
#include <QTimer>
#include <memory>

namespace peerchat::transfers {

class FileTransferManager : public IFileTransfer {
    Q_OBJECT
public:
    static constexpr qint64 CHUNK_SIZE = 64 * 1024; // 64 KB chunks

    FileTransferManager(networking::IPeerTransport* transport,
                        storage::IMessageStore* store,
                        identity::IIdentityProvider* identityProvider,
                        const QString& downloadDir,
                        QObject* parent = nullptr);
    ~FileTransferManager() override;

    QString startSendFile(const QString& peerId, const QString& localFilePath) override;
    bool acceptFileOffer(const QString& transferId) override;
    void rejectFileOffer(const QString& transferId) override;

    void cancelTransfer(const QString& transferId) override;
    void pauseTransfer(const QString& transferId) override;
    void resumeTransfer(const QString& transferId) override;

    void setDownloadDirectory(const QString& dir);
    QString downloadDirectory() const;

    // Handle incoming protocol messages relating to file transfer
    void handleProtocolMessage(const messaging::ProtocolMessage& msg);

private slots:
    void sendNextChunks();

private:
    struct ActiveOutgoing {
        TransferInfo info;
        std::unique_ptr<QFile> file;
        int nextChunkIndex{0};
        bool isPaused{false};
    };

    struct ActiveIncoming {
        TransferInfo info;
        std::unique_ptr<QFile> partFile;
        QString targetFinalPath;
        bool isPaused{false};
    };

    QString resolveUniqueFilePath(const QString& directory, const QString& filename);
    void finishIncomingTransfer(const QString& transferId);

    networking::IPeerTransport* m_transport{nullptr};
    storage::IMessageStore* m_store{nullptr};
    identity::IIdentityProvider* m_identityProvider{nullptr};
    QString m_downloadDir;

    QMap<QString, std::shared_ptr<ActiveOutgoing>> m_outgoingTransfers;
    QMap<QString, std::shared_ptr<ActiveIncoming>> m_incomingTransfers;
    QMap<QString, TransferInfo> m_pendingIncomingOffers;

    QTimer* m_outgoingChunkTimer{nullptr};
};

}  // namespace peerchat::transfers
