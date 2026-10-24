#include <gtest/gtest.h>
#include <QUuid>
#include "../../core/messaging/ProtocolMessage.hpp"

using namespace peerchat::messaging;

static QString dummyPeerId(char fill) {
    return QString("peer:ed25519:") + QString(64, fill);
}

TEST(ProtocolTest, TextMessageValidSerialization) {
    ProtocolMessage msg;
    msg.version = 1;
    msg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    msg.type = MessageType::Text;
    msg.senderId = dummyPeerId('a');
    msg.recipientId = dummyPeerId('b');
    msg.timestamp = 1729750000;
    msg.payload["text"] = "Valid message text";

    QByteArray json = msg.toJson();
    EXPECT_FALSE(json.isEmpty());

    auto parsed = ProtocolMessage::fromJson(json);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->version, 1);
    EXPECT_EQ(parsed->id, msg.id);
    EXPECT_EQ(parsed->type, MessageType::Text);
    EXPECT_EQ(parsed->senderId, msg.senderId);
    EXPECT_EQ(parsed->recipientId, msg.recipientId);
    EXPECT_EQ(parsed->payload.value("text").toString(), "Valid message text");
}

TEST(ProtocolTest, RejectInvalidVersionAndMalformed) {
    // Malformed JSON
    EXPECT_FALSE(ProtocolMessage::fromJson("{not json").has_value());

    // Empty buffer
    EXPECT_FALSE(ProtocolMessage::fromJson("").has_value());

    // Version != 1
    ProtocolMessage msg;
    msg.version = 99;
    msg.id = "msg-1";
    msg.type = MessageType::Text;
    msg.senderId = dummyPeerId('a');
    msg.recipientId = dummyPeerId('b');
    msg.timestamp = 1729750000;
    msg.payload["text"] = "hi";
    EXPECT_FALSE(msg.isValid());

    // Missing text
    msg.version = 1;
    msg.payload = QJsonObject();
    EXPECT_FALSE(msg.isValid());
}

TEST(ProtocolTest, DeliveryAckSerialization) {
    ProtocolMessage ack;
    ack.version = 1;
    ack.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ack.type = MessageType::DeliveryAck;
    ack.senderId = dummyPeerId('b');
    ack.recipientId = dummyPeerId('a');
    ack.timestamp = 1729750010;
    ack.payload["ack_message_id"] = "original-msg-123";
    ack.payload["status"] = "delivered";

    QByteArray json = ack.toJson();
    auto parsed = ProtocolMessage::fromJson(json);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->type, MessageType::DeliveryAck);
    EXPECT_EQ(parsed->payload.value("ack_message_id").toString(), "original-msg-123");
}
