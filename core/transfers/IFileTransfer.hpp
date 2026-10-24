#pragma once

#include <QObject>
#include <QString>

namespace peerchat::transfers {

enum class TransferState {
    Pending,
    Active,
    Paused,
    Completed,
    Failed,
    Cancelled
};

QString transferStateToString(TransferState state);
TransferState stringToTransferState(const QString& str);

struct TransferInfo {
    QString transferId;
    QString peerId;
    QString fileName;
    QString filePath;
    qint64 fileSize{0};
    QString sha256;
    qint64 transferredBytes{0};
    TransferState state{TransferState::Pending};
    bool isOutgoing{true};
    int totalChunks{0};
    int completedChunks{0};
    QString mimeType;
};

class IFileTransfer : public QObject {
    Q_OBJECT
public:
    explicit IFileTransfer(QObject* parent = nullptr) : QObject(parent) {}
    ~IFileTransfer() override = default;

    // Send a file to peer
    virtual QString startSendFile(const QString& peerId, const QString& localFilePath) = 0;

    // Accept or reject an incoming file offer
    virtual bool acceptFileOffer(const QString& transferId) = 0;
    virtual void rejectFileOffer(const QString& transferId) = 0;

    // Control
    virtual void cancelTransfer(const QString& transferId) = 0;
    virtual void pauseTransfer(const QString& transferId) = 0;
    virtual void resumeTransfer(const QString& transferId) = 0;

    // Utility: Filename sanitization to protect against directory traversal
    static QString sanitizeFilename(const QString& rawName);

signals:
    void transferOffered(const TransferInfo& info);
    void transferProgress(const QString& transferId, qint64 bytesTransferred, qint64 totalBytes);
    void transferCompleted(const QString& transferId, const QString& finalFilePath);
    void transferFailed(const QString& transferId, const QString& reason);
    void transferCancelled(const QString& transferId);
};

}  // namespace peerchat::transfers
