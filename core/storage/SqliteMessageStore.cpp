#include "SqliteMessageStore.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>

namespace peerchat::storage {

QString messageStatusToString(MessageStatus status) {
    switch (status) {
        case MessageStatus::Sending: return QStringLiteral("sending");
        case MessageStatus::Sent: return QStringLiteral("sent");
        case MessageStatus::Delivered: return QStringLiteral("delivered");
        case MessageStatus::Read: return QStringLiteral("read");
        case MessageStatus::Failed: default: return QStringLiteral("failed");
    }
}

MessageStatus stringToMessageStatus(const QString& str) {
    if (str == QLatin1String("sending")) return MessageStatus::Sending;
    if (str == QLatin1String("sent")) return MessageStatus::Sent;
    if (str == QLatin1String("delivered")) return MessageStatus::Delivered;
    if (str == QLatin1String("read")) return MessageStatus::Read;
    return MessageStatus::Failed;
}

SqliteMessageStore::SqliteMessageStore(QObject* parent)
    : IMessageStore(parent) {
    m_connectionName = QStringLiteral("peerchat_db_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

SqliteMessageStore::~SqliteMessageStore() {
    close();
}

bool SqliteMessageStore::open(const QString& dbPath) {
    QMutexLocker locker(&m_mutex);
    if (m_isOpen) {
        return true;
    }

    QFileInfo fi(dbPath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        return false;
    }

    m_isOpen = true;
    return createTables();
}

void SqliteMessageStore::close() {
    QMutexLocker locker(&m_mutex);
    if (m_isOpen) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
        m_isOpen = false;
    }
}

bool SqliteMessageStore::createTables() {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);

    // WAL mode and Foreign Keys
    q.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));

    // Contacts table
    bool ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS contacts ("
        "  peer_id TEXT PRIMARY KEY NOT NULL,"
        "  display_name TEXT NOT NULL,"
        "  trust_level TEXT NOT NULL DEFAULT 'untrusted',"
        "  last_seen INTEGER NOT NULL DEFAULT 0,"
        "  is_blocked INTEGER NOT NULL DEFAULT 0"
        ");"
    ));
    if (!ok) return false;

    // Messages table
    ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  conversation_id TEXT NOT NULL,"
        "  sender_id TEXT NOT NULL,"
        "  recipient_id TEXT NOT NULL,"
        "  type TEXT NOT NULL DEFAULT 'text',"
        "  content TEXT,"
        "  media_path TEXT,"
        "  status TEXT NOT NULL DEFAULT 'sending',"
        "  timestamp INTEGER NOT NULL,"
        "  delivered_at INTEGER NOT NULL DEFAULT 0"
        ");"
    ));
    if (!ok) return false;

    // Create index on conversation_id and timestamp
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_msg_conv_time ON messages(conversation_id, timestamp);"));

    // Transfers table
    ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS transfers ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  message_id TEXT NOT NULL,"
        "  file_name TEXT NOT NULL,"
        "  file_path TEXT NOT NULL,"
        "  file_size INTEGER NOT NULL DEFAULT 0,"
        "  sha256 TEXT NOT NULL,"
        "  transferred_bytes INTEGER NOT NULL DEFAULT 0,"
        "  status TEXT NOT NULL DEFAULT 'pending',"
        "  direction TEXT NOT NULL DEFAULT 'outgoing',"
        "  timestamp INTEGER NOT NULL DEFAULT 0"
        ");"
    ));
    return ok;
}

bool SqliteMessageStore::upsertContact(const Contact& contact) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO contacts (peer_id, display_name, trust_level, last_seen, is_blocked) "
        "VALUES (:peer_id, :display_name, :trust_level, :last_seen, :is_blocked) "
        "ON CONFLICT(peer_id) DO UPDATE SET "
        "  display_name = excluded.display_name, "
        "  trust_level = excluded.trust_level, "
        "  last_seen = excluded.last_seen, "
        "  is_blocked = excluded.is_blocked;"
    ));
    q.bindValue(":peer_id", contact.peerId);
    q.bindValue(":display_name", contact.displayName);
    q.bindValue(":trust_level", contact.trustLevel);
    q.bindValue(":last_seen", contact.lastSeen);
    q.bindValue(":is_blocked", contact.isBlocked ? 1 : 0);

    if (q.exec()) {
        emit contactUpdated(contact);
        return true;
    }
    return false;
}

std::optional<Contact> SqliteMessageStore::getContact(const QString& peerId) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return std::nullopt;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT peer_id, display_name, trust_level, last_seen, is_blocked FROM contacts WHERE peer_id = :peer_id"));
    q.bindValue(":peer_id", peerId);

    if (q.exec() && q.next()) {
        Contact c;
        c.peerId = q.value(0).toString();
        c.displayName = q.value(1).toString();
        c.trustLevel = q.value(2).toString();
        c.lastSeen = q.value(3).toLongLong();
        c.isBlocked = q.value(4).toInt() == 1;
        return c;
    }
    return std::nullopt;
}

QList<Contact> SqliteMessageStore::getAllContacts() {
    QMutexLocker locker(&m_mutex);
    QList<Contact> list;
    if (!m_isOpen) return list;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral("SELECT peer_id, display_name, trust_level, last_seen, is_blocked FROM contacts ORDER BY last_seen DESC"), db);
    while (q.next()) {
        Contact c;
        c.peerId = q.value(0).toString();
        c.displayName = q.value(1).toString();
        c.trustLevel = q.value(2).toString();
        c.lastSeen = q.value(3).toLongLong();
        c.isBlocked = q.value(4).toInt() == 1;
        list.append(c);
    }
    return list;
}

bool SqliteMessageStore::deleteContact(const QString& peerId) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM contacts WHERE peer_id = :peer_id"));
    q.bindValue(":peer_id", peerId);
    return q.exec();
}

bool SqliteMessageStore::saveMessage(const StoredMessage& message) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO messages (id, conversation_id, sender_id, recipient_id, type, content, media_path, status, timestamp, delivered_at) "
        "VALUES (:id, :conversation_id, :sender_id, :recipient_id, :type, :content, :media_path, :status, :timestamp, :delivered_at) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  status = excluded.status, "
        "  delivered_at = excluded.delivered_at;"
    ));
    q.bindValue(":id", message.id);
    q.bindValue(":conversation_id", message.conversationId);
    q.bindValue(":sender_id", message.senderId);
    q.bindValue(":recipient_id", message.recipientId);
    q.bindValue(":type", message.type);
    q.bindValue(":content", message.content);
    q.bindValue(":media_path", message.mediaPath);
    q.bindValue(":status", messageStatusToString(message.status));
    q.bindValue(":timestamp", message.timestamp);
    q.bindValue(":delivered_at", message.deliveredAt);

    if (q.exec()) {
        emit messageSaved(message);
        return true;
    }
    return false;
}

bool SqliteMessageStore::updateMessageStatus(const QString& messageId, MessageStatus status, qint64 deliveredAt) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (deliveredAt > 0) {
        q.prepare(QStringLiteral("UPDATE messages SET status = :status, delivered_at = :delivered_at WHERE id = :id"));
        q.bindValue(":status", messageStatusToString(status));
        q.bindValue(":delivered_at", deliveredAt);
        q.bindValue(":id", messageId);
    } else {
        q.prepare(QStringLiteral("UPDATE messages SET status = :status WHERE id = :id"));
        q.bindValue(":status", messageStatusToString(status));
        q.bindValue(":id", messageId);
    }

    if (q.exec()) {
        emit messageStatusChanged(messageId, status);
        return true;
    }
    return false;
}

std::optional<StoredMessage> SqliteMessageStore::getMessage(const QString& messageId) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return std::nullopt;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id, conversation_id, sender_id, recipient_id, type, content, media_path, status, timestamp, delivered_at FROM messages WHERE id = :id"));
    q.bindValue(":id", messageId);

    if (q.exec() && q.next()) {
        StoredMessage m;
        m.id = q.value(0).toString();
        m.conversationId = q.value(1).toString();
        m.senderId = q.value(2).toString();
        m.recipientId = q.value(3).toString();
        m.type = q.value(4).toString();
        m.content = q.value(5).toString();
        m.mediaPath = q.value(6).toString();
        m.status = stringToMessageStatus(q.value(7).toString());
        m.timestamp = q.value(8).toLongLong();
        m.deliveredAt = q.value(9).toLongLong();
        return m;
    }
    return std::nullopt;
}

QList<StoredMessage> SqliteMessageStore::getMessagesForConversation(const QString& conversationId, int limit, int offset) {
    QMutexLocker locker(&m_mutex);
    QList<StoredMessage> list;
    if (!m_isOpen) return list;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, conversation_id, sender_id, recipient_id, type, content, media_path, status, timestamp, delivered_at "
        "FROM messages WHERE conversation_id = :conversation_id "
        "ORDER BY timestamp ASC LIMIT :limit OFFSET :offset"
    ));
    q.bindValue(":conversation_id", conversationId);
    q.bindValue(":limit", limit);
    q.bindValue(":offset", offset);

    if (q.exec()) {
        while (q.next()) {
            StoredMessage m;
            m.id = q.value(0).toString();
            m.conversationId = q.value(1).toString();
            m.senderId = q.value(2).toString();
            m.recipientId = q.value(3).toString();
            m.type = q.value(4).toString();
            m.content = q.value(5).toString();
            m.mediaPath = q.value(6).toString();
            m.status = stringToMessageStatus(q.value(7).toString());
            m.timestamp = q.value(8).toLongLong();
            m.deliveredAt = q.value(9).toLongLong();
            list.append(m);
        }
    }
    return list;
}

bool SqliteMessageStore::saveTransfer(const StoredTransfer& transfer) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO transfers (id, message_id, file_name, file_path, file_size, sha256, transferred_bytes, status, direction, timestamp) "
        "VALUES (:id, :message_id, :file_name, :file_path, :file_size, :sha256, :transferred_bytes, :status, :direction, :timestamp) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  transferred_bytes = excluded.transferred_bytes, "
        "  status = excluded.status;"
    ));
    q.bindValue(":id", transfer.id);
    q.bindValue(":message_id", transfer.messageId);
    q.bindValue(":file_name", transfer.fileName);
    q.bindValue(":file_path", transfer.filePath);
    q.bindValue(":file_size", transfer.fileSize);
    q.bindValue(":sha256", transfer.sha256);
    q.bindValue(":transferred_bytes", transfer.transferredBytes);
    q.bindValue(":status", transfer.status);
    q.bindValue(":direction", transfer.direction);
    q.bindValue(":timestamp", transfer.timestamp);

    if (q.exec()) {
        emit transferUpdated(transfer);
        return true;
    }
    return false;
}

bool SqliteMessageStore::updateTransferProgress(const QString& transferId, qint64 transferredBytes, const QString& status) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE transfers SET transferred_bytes = :transferred_bytes, status = :status WHERE id = :id"));
    q.bindValue(":transferred_bytes", transferredBytes);
    q.bindValue(":status", status);
    q.bindValue(":id", transferId);

    if (q.exec()) {
        auto opt = getTransfer(transferId);
        if (opt.has_value()) {
            emit transferUpdated(opt.value());
        }
        return true;
    }
    return false;
}

std::optional<StoredTransfer> SqliteMessageStore::getTransfer(const QString& transferId) {
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) return std::nullopt;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id, message_id, file_name, file_path, file_size, sha256, transferred_bytes, status, direction, timestamp FROM transfers WHERE id = :id"));
    q.bindValue(":id", transferId);

    if (q.exec() && q.next()) {
        StoredTransfer t;
        t.id = q.value(0).toString();
        t.messageId = q.value(1).toString();
        t.fileName = q.value(2).toString();
        t.filePath = q.value(3).toString();
        t.fileSize = q.value(4).toLongLong();
        t.sha256 = q.value(5).toString();
        t.transferredBytes = q.value(6).toLongLong();
        t.status = q.value(7).toString();
        t.direction = q.value(8).toString();
        t.timestamp = q.value(9).toLongLong();
        return t;
    }
    return std::nullopt;
}

QList<StoredTransfer> SqliteMessageStore::getAllTransfers() {
    QMutexLocker locker(&m_mutex);
    QList<StoredTransfer> list;
    if (!m_isOpen) return list;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(QStringLiteral("SELECT id, message_id, file_name, file_path, file_size, sha256, transferred_bytes, status, direction, timestamp FROM transfers ORDER BY timestamp DESC"), db);
    while (q.next()) {
        StoredTransfer t;
        t.id = q.value(0).toString();
        t.messageId = q.value(1).toString();
        t.fileName = q.value(2).toString();
        t.filePath = q.value(3).toString();
        t.fileSize = q.value(4).toLongLong();
        t.sha256 = q.value(5).toString();
        t.transferredBytes = q.value(6).toLongLong();
        t.status = q.value(7).toString();
        t.direction = q.value(8).toString();
        t.timestamp = q.value(9).toLongLong();
        list.append(t);
    }
    return list;
}

}  // namespace peerchat::storage
