#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <optional>

namespace peerchat::storage {

enum class MessageStatus {
    Sending,
    Sent,
    Delivered,
    Read,
    Failed
};

QString messageStatusToString(MessageStatus status);
MessageStatus stringToMessageStatus(const QString& str);

struct Contact {
    QString peerId;
    QString displayName;
    QString trustLevel{"untrusted"}; // "untrusted", "verified", "pinned"
    qint64 lastSeen{0};
    bool isBlocked{false};
};

struct StoredMessage {
    QString id;
    QString conversationId;
    QString senderId;
    QString recipientId;
    QString type;
    QString content;
    QString mediaPath;
    MessageStatus status{MessageStatus::Sending};
    qint64 timestamp{0};
    qint64 deliveredAt{0};
};

struct StoredTransfer {
    QString id;
    QString messageId;
    QString fileName;
    QString filePath;
    qint64 fileSize{0};
    QString sha256;
    qint64 transferredBytes{0};
    QString status{"pending"};
    QString direction{"outgoing"}; // "outgoing" or "incoming"
    qint64 timestamp{0};
};

class IMessageStore : public QObject {
    Q_OBJECT
public:
    explicit IMessageStore(QObject* parent = nullptr) : QObject(parent) {}
    ~IMessageStore() override = default;

    virtual bool open(const QString& dbPath) = 0;
    virtual void close() = 0;

    // Contacts
    virtual bool upsertContact(const Contact& contact) = 0;
    virtual std::optional<Contact> getContact(const QString& peerId) = 0;
    virtual QList<Contact> getAllContacts() = 0;
    virtual bool deleteContact(const QString& peerId) = 0;

    // Messages
    virtual bool saveMessage(const StoredMessage& message) = 0;
    virtual bool updateMessageStatus(const QString& messageId, MessageStatus status, qint64 deliveredAt = 0) = 0;
    virtual std::optional<StoredMessage> getMessage(const QString& messageId) = 0;
    virtual QList<StoredMessage> getMessagesForConversation(const QString& conversationId, int limit = 50, int offset = 0) = 0;

    // Transfers
    virtual bool saveTransfer(const StoredTransfer& transfer) = 0;
    virtual bool updateTransferProgress(const QString& transferId, qint64 transferredBytes, const QString& status) = 0;
    virtual std::optional<StoredTransfer> getTransfer(const QString& transferId) = 0;
    virtual QList<StoredTransfer> getAllTransfers() = 0;

signals:
    void contactUpdated(const Contact& contact);
    void messageSaved(const StoredMessage& message);
    void messageStatusChanged(const QString& messageId, MessageStatus newStatus);
    void transferUpdated(const StoredTransfer& transfer);
};

}  // namespace peerchat::storage
