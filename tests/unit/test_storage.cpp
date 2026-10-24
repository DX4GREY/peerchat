#include <gtest/gtest.h>
#include <QTemporaryDir>
#include "../../core/storage/SqliteMessageStore.hpp"

using namespace peerchat::storage;

TEST(StorageTest, ContactsCrud) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    QString dbPath = tempDir.filePath("test.db");

    SqliteMessageStore store;
    ASSERT_TRUE(store.open(dbPath));

    Contact c1;
    c1.peerId = "peer:ed25519:1111111111111111111111111111111111111111111111111111111111111111";
    c1.displayName = "Alice";
    c1.trustLevel = "verified";
    c1.lastSeen = 123456789;
    c1.isBlocked = false;

    EXPECT_TRUE(store.upsertContact(c1));

    auto retrieved = store.getContact(c1.peerId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->displayName, "Alice");
    EXPECT_EQ(retrieved->trustLevel, "verified");

    // Update display name
    c1.displayName = "Alice Cooper";
    EXPECT_TRUE(store.upsertContact(c1));
    retrieved = store.getContact(c1.peerId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->displayName, "Alice Cooper");

    // Delete
    EXPECT_TRUE(store.deleteContact(c1.peerId));
    EXPECT_FALSE(store.getContact(c1.peerId).has_value());
}

TEST(StorageTest, MessagesPersistenceAndStatusUpdate) {
    QTemporaryDir tempDir;
    QString dbPath = tempDir.filePath("test_messages.db");

    SqliteMessageStore store;
    ASSERT_TRUE(store.open(dbPath));

    StoredMessage m;
    m.id = "msg-001";
    m.conversationId = "conv-1";
    m.senderId = "peer:ed25519:aaa";
    m.recipientId = "peer:ed25519:bbb";
    m.type = "text";
    m.content = "Unit testing message";
    m.status = MessageStatus::Sending;
    m.timestamp = 1000;

    EXPECT_TRUE(store.saveMessage(m));

    auto retrieved = store.getMessage("msg-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->content, "Unit testing message");
    EXPECT_EQ(retrieved->status, MessageStatus::Sending);

    // Update status to Delivered
    EXPECT_TRUE(store.updateMessageStatus("msg-001", MessageStatus::Delivered, 1005));
    retrieved = store.getMessage("msg-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->status, MessageStatus::Delivered);
    EXPECT_EQ(retrieved->deliveredAt, 1005);

    auto list = store.getMessagesForConversation("conv-1");
    EXPECT_EQ(list.size(), 1);
}

TEST(StorageTest, TransfersPersistence) {
    QTemporaryDir tempDir;
    QString dbPath = tempDir.filePath("test_transfers.db");

    SqliteMessageStore store;
    ASSERT_TRUE(store.open(dbPath));

    StoredTransfer t;
    t.id = "tx-100";
    t.messageId = "msg-tx-100";
    t.fileName = "document.pdf";
    t.filePath = "/tmp/document.pdf";
    t.fileSize = 1048576;
    t.sha256 = "dummyhash";
    t.transferredBytes = 0;
    t.status = "pending";
    t.direction = "outgoing";
    t.timestamp = 2000;

    EXPECT_TRUE(store.saveTransfer(t));

    EXPECT_TRUE(store.updateTransferProgress("tx-100", 524288, "active"));
    auto retrieved = store.getTransfer("tx-100");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->transferredBytes, 524288);
    EXPECT_EQ(retrieved->status, "active");
}
