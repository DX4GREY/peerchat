#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace peerchat::messaging {

enum class MessageType {
    Text,
    DeliveryAck,
    FileOffer,
    FileChunk,
    FileControl,
    Typing,
    Presence,
    Unknown
};

QString messageTypeToString(MessageType type);
MessageType stringToMessageType(const QString& str);

struct ProtocolMessage {
    int version{1};
    QString id;
    MessageType type{MessageType::Text};
    QString senderId;
    QString recipientId;
    qint64 timestamp{0};
    QString conversationId;
    QJsonObject payload;
    QByteArray signature;

    // Serialization
    QByteArray toJson(bool compact = true) const;
    static std::optional<ProtocolMessage> fromJson(const QByteArray& data);

    // Validation
    bool isValid() const;

    // Helper: compute canonical bytes to sign
    QByteArray canonicalPayloadBytes() const;
};

}  // namespace peerchat::messaging
