#pragma once

#include "IMessageStore.hpp"
#include <QSqlDatabase>
#include <QRecursiveMutex>

namespace peerchat::storage {

class SqliteMessageStore : public IMessageStore {
    Q_OBJECT
public:
    explicit SqliteMessageStore(QObject* parent = nullptr);
    ~SqliteMessageStore() override;

    bool open(const QString& dbPath) override;
    void close() override;

    // Contacts
    bool upsertContact(const Contact& contact) override;
    std::optional<Contact> getContact(const QString& peerId) override;
    QList<Contact> getAllContacts() override;
    bool deleteContact(const QString& peerId) override;

    // Messages
    bool saveMessage(const StoredMessage& message) override;
    bool updateMessageStatus(const QString& messageId, MessageStatus status, qint64 deliveredAt = 0) override;
    std::optional<StoredMessage> getMessage(const QString& messageId) override;
    QList<StoredMessage> getMessagesForConversation(const QString& conversationId, int limit = 50, int offset = 0) override;

    // Transfers
    bool saveTransfer(const StoredTransfer& transfer) override;
    bool updateTransferProgress(const QString& transferId, qint64 transferredBytes, const QString& status) override;
    std::optional<StoredTransfer> getTransfer(const QString& transferId) override;
    QList<StoredTransfer> getAllTransfers() override;

private:
    bool createTables();

    QString m_connectionName;
    QRecursiveMutex m_mutex;
    bool m_isOpen{false};
};

}  // namespace peerchat::storage
