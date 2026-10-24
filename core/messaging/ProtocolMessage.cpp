#include "ProtocolMessage.hpp"

#include <QJsonDocument>
#include <QUuid>

namespace peerchat::messaging {

QString messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::Text: return QStringLiteral("text");
        case MessageType::DeliveryAck: return QStringLiteral("delivery_ack");
        case MessageType::FileOffer: return QStringLiteral("file_offer");
        case MessageType::FileChunk: return QStringLiteral("file_chunk");
        case MessageType::FileControl: return QStringLiteral("file_control");
        case MessageType::Typing: return QStringLiteral("typing");
        case MessageType::Presence: return QStringLiteral("presence");
        case MessageType::Unknown: default: return QStringLiteral("unknown");
    }
}

MessageType stringToMessageType(const QString& str) {
    if (str == QLatin1String("text")) return MessageType::Text;
    if (str == QLatin1String("delivery_ack")) return MessageType::DeliveryAck;
    if (str == QLatin1String("file_offer")) return MessageType::FileOffer;
    if (str == QLatin1String("file_chunk")) return MessageType::FileChunk;
    if (str == QLatin1String("file_control")) return MessageType::FileControl;
    if (str == QLatin1String("typing")) return MessageType::Typing;
    if (str == QLatin1String("presence")) return MessageType::Presence;
    return MessageType::Unknown;
}

QByteArray ProtocolMessage::canonicalPayloadBytes() const {
    // Canonical data to sign: version|id|type|sender|recipient|timestamp|payload_json
    QByteArray canonical;
    canonical.append(QByteArray::number(version));
    canonical.append('|');
    canonical.append(id.toUtf8());
    canonical.append('|');
    canonical.append(messageTypeToString(type).toUtf8());
    canonical.append('|');
    canonical.append(senderId.toUtf8());
    canonical.append('|');
    canonical.append(recipientId.toUtf8());
    canonical.append('|');
    canonical.append(QByteArray::number(timestamp));
    canonical.append('|');
    QJsonDocument payloadDoc(payload);
    canonical.append(payloadDoc.toJson(QJsonDocument::Compact));
    return canonical;
}

bool ProtocolMessage::isValid() const {
    if (version != 1) return false;
    if (id.isEmpty() || id.length() > 64) return false;
    if (type == MessageType::Unknown) return false;
    if (!senderId.startsWith("peer:ed25519:") || senderId.length() != 77) return false;
    if (!recipientId.startsWith("peer:ed25519:") || recipientId.length() != 77) return false;
    if (timestamp <= 0) return false;

    // Type-specific validation
    switch (type) {
        case MessageType::Text: {
            QString text = payload.value("text").toString();
            if (text.isEmpty() || text.toUtf8().size() > 65536) return false;
            break;
        }
        case MessageType::DeliveryAck: {
            QString ackId = payload.value("ack_message_id").toString();
            if (ackId.isEmpty()) return false;
            break;
        }
        case MessageType::FileOffer: {
            QString transferId = payload.value("transfer_id").toString();
            QString filename = payload.value("filename").toString();
            qint64 totalSize = payload.value("total_size").toInteger();
            if (transferId.isEmpty() || filename.isEmpty() || totalSize < 0) return false;
            break;
        }
        case MessageType::FileChunk: {
            QString transferId = payload.value("transfer_id").toString();
            if (transferId.isEmpty()) return false;
            if (!payload.contains("chunk_index")) return false;
            QString chunkData = payload.value("chunk_data").toString();
            // Maximum 64KB binary chunk is ~88KB Base64
            if (chunkData.size() > 100000) return false;
            break;
        }
        default:
            break;
    }

    return true;
}

QByteArray ProtocolMessage::toJson(bool compact) const {
    QJsonObject obj;
    obj["version"] = version;
    obj["id"] = id;
    obj["type"] = messageTypeToString(type);
    obj["sender_id"] = senderId;
    obj["recipient_id"] = recipientId;
    obj["timestamp"] = timestamp;
    obj["conversation_id"] = conversationId.isEmpty() ? (senderId < recipientId ? senderId + "_" + recipientId : recipientId + "_" + senderId) : conversationId;
    obj["payload"] = payload;
    if (!signature.isEmpty()) {
        obj["signature"] = QString::fromUtf8(signature.toHex());
    }

    QJsonDocument doc(obj);
    return doc.toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

std::optional<ProtocolMessage> ProtocolMessage::fromJson(const QByteArray& data) {
    // Upper bound sanity check (10MB)
    if (data.isEmpty() || data.size() > 10 * 1024 * 1024) {
        return std::nullopt;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    QJsonObject obj = doc.object();
    ProtocolMessage msg;
    msg.version = obj.value("version").toInt(1);
    msg.id = obj.value("id").toString();
    msg.type = stringToMessageType(obj.value("type").toString());
    msg.senderId = obj.value("sender_id").toString();
    msg.recipientId = obj.value("recipient_id").toString();
    msg.timestamp = obj.value("timestamp").toInteger(0);
    msg.conversationId = obj.value("conversation_id").toString();
    msg.payload = obj.value("payload").toObject();
    
    if (obj.contains("signature")) {
        msg.signature = QByteArray::fromHex(obj.value("signature").toString().toUtf8());
    }

    if (!msg.isValid()) {
        return std::nullopt;
    }

    return msg;
}

}  // namespace peerchat::messaging
