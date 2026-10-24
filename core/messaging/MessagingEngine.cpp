#include "MessagingEngine.hpp"

#include <QDateTime>
#include <QDir>
#include <QUuid>

#include "../transfers/FileTransferManager.hpp"

namespace peerchat::messaging {

MessagingEngine::MessagingEngine(identity::IIdentityProvider* identityProvider,
                                 networking::IPeerTransport* directTransport,
                                 networking::IPeerTransport* relayTransport,
                                 discovery::IPeerDiscovery* discovery,
                                 storage::IMessageStore* store,
                                 transfers::IFileTransfer* transferManager, QObject* parent)
    : QObject(parent),
      m_identityProvider(identityProvider),
      m_directTransport(directTransport),
      m_relayTransport(relayTransport),
      m_discovery(discovery),
      m_store(store),
      m_transferManager(transferManager) {
    if (m_directTransport) {
        connect(m_directTransport, &networking::IPeerTransport::packetReceived, this,
                &MessagingEngine::onPacketReceived);
        connect(m_directTransport, &networking::IPeerTransport::peerConnected, this,
                &MessagingEngine::onTransportPeerConnected);
        connect(m_directTransport, &networking::IPeerTransport::routeChanged, this,
                &MessagingEngine::onTransportRouteChanged);
    }

    if (m_relayTransport) {
        connect(m_relayTransport, &networking::IPeerTransport::packetReceived, this,
                &MessagingEngine::onPacketReceived);
        connect(m_relayTransport, &networking::IPeerTransport::peerConnected, this,
                &MessagingEngine::onTransportPeerConnected);
        connect(m_relayTransport, &networking::IPeerTransport::routeChanged, this,
                &MessagingEngine::onTransportRouteChanged);
    }

    if (m_discovery) {
        connect(m_discovery, &discovery::IPeerDiscovery::peerDiscovered, this,
                &MessagingEngine::onPeerDiscovered);
    }
}

MessagingEngine::~MessagingEngine() = default;

bool MessagingEngine::initialize(const QString& dataDir, const QString& profileName) {
    m_dataDir = dataDir;
    QDir dir(m_dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString keyPath = dir.filePath(QStringLiteral("identity.key"));
    QString dbPath = dir.filePath(QStringLiteral("peerchat.db"));

    if (m_identityProvider && !m_identityProvider->loadOrCreate(keyPath, profileName)) {
        return false;
    }

    if (m_store && !m_store->open(dbPath)) {
        return false;
    }

    if (m_directTransport) {
        m_directTransport->startListening();
    }

    if (m_discovery) {
        m_discovery->start();
    }

    return true;
}

QString MessagingEngine::sendTextMessage(const QString& recipientPeerId, const QString& text) {
    if (!m_identityProvider)
        return {};

    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    ProtocolMessage protoMsg;
    protoMsg.version = 1;
    protoMsg.id = messageId;
    protoMsg.type = MessageType::Text;
    protoMsg.senderId = m_identityProvider->peerId();
    protoMsg.recipientId = recipientPeerId;
    protoMsg.timestamp = now;
    protoMsg.payload["text"] = text;
    protoMsg.signature = m_identityProvider->sign(protoMsg.canonicalPayloadBytes());

    // Compute conversation ID
    QString convId = protoMsg.senderId < protoMsg.recipientId
                         ? protoMsg.senderId + "_" + protoMsg.recipientId
                         : protoMsg.recipientId + "_" + protoMsg.senderId;

    // Save in local storage
    storage::StoredMessage sm;
    sm.id = messageId;
    sm.conversationId = convId;
    sm.senderId = protoMsg.senderId;
    sm.recipientId = recipientPeerId;
    sm.type = QStringLiteral("text");
    sm.content = text;
    sm.status = storage::MessageStatus::Sending;
    sm.timestamp = now;

    if (m_store) {
        m_store->saveMessage(sm);
    }

    QByteArray packet = protoMsg.toJson();
    bool sent = false;

    // Direct transport first
    if (m_directTransport && m_directTransport->isConnected(recipientPeerId)) {
        sent = m_directTransport->sendPacket(recipientPeerId, packet);
    } else if (m_relayTransport && m_relayTransport->isConnected(recipientPeerId)) {
        sent = m_relayTransport->sendPacket(recipientPeerId, packet);
    }

    if (sent && m_store) {
        m_store->updateMessageStatus(messageId, storage::MessageStatus::Sent);
    }

    return messageId;
}

QString MessagingEngine::sendFile(const QString& recipientPeerId, const QString& filePath) {
    if (m_transferManager) {
        return m_transferManager->startSendFile(recipientPeerId, filePath);
    }
    return {};
}

bool MessagingEngine::addOrUpdateContact(const QString& peerId, const QString& displayName,
                                         const QString& trustLevel) {
    if (m_store) {
        storage::Contact c;
        c.peerId = peerId;
        c.displayName = displayName;
        c.trustLevel = trustLevel;
        c.lastSeen = QDateTime::currentMSecsSinceEpoch();
        c.isBlocked = false;
        return m_store->upsertContact(c);
    }
    return false;
}

QList<storage::Contact> MessagingEngine::getContacts() const {
    if (m_store) {
        return m_store->getAllContacts();
    }
    return {};
}

networking::RouteType MessagingEngine::getRoute(const QString& peerId) const {
    if (m_directTransport && m_directTransport->isConnected(peerId)) {
        return networking::RouteType::Direct;
    }
    if (m_relayTransport && m_relayTransport->isConnected(peerId)) {
        return networking::RouteType::Relay;
    }
    if (m_directTransport &&
        m_directTransport->getRouteType(peerId) == networking::RouteType::Connecting) {
        return networking::RouteType::Connecting;
    }
    return networking::RouteType::Offline;
}

void MessagingEngine::connectToPeerDirect(const QString& peerId, const QString& host,
                                          quint16 port) {
    if (m_directTransport) {
        m_directTransport->connectToPeer(peerId, host, port);
    }
}

void MessagingEngine::onPacketReceived(const QString& peerId, const QByteArray& packet) {
    auto optMsg = ProtocolMessage::fromJson(packet);
    if (!optMsg.has_value()) {
        return;  // Drop malformed
    }

    const ProtocolMessage& msg = optMsg.value();

    // Verify sender signature
    if (!m_identityProvider || !m_identityProvider->verifyPeerSignature(
                                   msg.senderId, msg.canonicalPayloadBytes(), msg.signature)) {
        return;  // Drop tampered message
    }

    // Replay/Duplicate check
    if (m_processedMessageIds.contains(msg.id)) {
        return;
    }
    m_processedMessageIds.insert(msg.id);
    if (m_processedMessageIds.size() > 5000) {
        m_processedMessageIds.clear();  // Keep bounded cache
    }

    switch (msg.type) {
        case MessageType::Text: {
            QString convId = msg.conversationId;
            if (convId.isEmpty()) {
                convId = msg.senderId < msg.recipientId ? msg.senderId + "_" + msg.recipientId
                                                        : msg.recipientId + "_" + msg.senderId;
            }

            storage::StoredMessage sm;
            sm.id = msg.id;
            sm.conversationId = convId;
            sm.senderId = msg.senderId;
            sm.recipientId = msg.recipientId;
            sm.type = QStringLiteral("text");
            sm.content = msg.payload.value("text").toString();
            sm.status = storage::MessageStatus::Delivered;
            sm.timestamp = msg.timestamp;
            sm.deliveredAt = QDateTime::currentMSecsSinceEpoch();

            if (m_store) {
                m_store->saveMessage(sm);
                // Also update contact last seen
                storage::Contact c;
                c.peerId = msg.senderId;
                c.displayName = msg.senderId.left(12);
                c.lastSeen = sm.deliveredAt;
                m_store->upsertContact(c);
            }

            emit messageReceived(sm);

            // Automatically send delivery acknowledgment back to sender
            sendDeliveryAck(msg.senderId, msg.id);
            break;
        }

        case MessageType::DeliveryAck: {
            QString ackId = msg.payload.value("ack_message_id").toString();
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (m_store) {
                m_store->updateMessageStatus(ackId, storage::MessageStatus::Delivered, now);
            }
            emit messageDelivered(ackId, now);
            break;
        }

        case MessageType::FileOffer:
        case MessageType::FileChunk:
        case MessageType::FileControl: {
            if (m_transferManager) {
                auto* ftm = qobject_cast<transfers::FileTransferManager*>(m_transferManager);
                if (ftm) {
                    ftm->handleProtocolMessage(msg);
                }
            }
            break;
        }

        default:
            break;
    }
}

void MessagingEngine::sendDeliveryAck(const QString& recipientPeerId, const QString& ackMessageId) {
    if (!m_identityProvider)
        return;

    ProtocolMessage ack;
    ack.version = 1;
    ack.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ack.type = MessageType::DeliveryAck;
    ack.senderId = m_identityProvider->peerId();
    ack.recipientId = recipientPeerId;
    ack.timestamp = QDateTime::currentMSecsSinceEpoch();
    ack.payload["ack_message_id"] = ackMessageId;
    ack.payload["status"] = QStringLiteral("delivered");
    ack.signature = m_identityProvider->sign(ack.canonicalPayloadBytes());

    QByteArray packet = ack.toJson();
    if (m_directTransport && m_directTransport->isConnected(recipientPeerId)) {
        m_directTransport->sendPacket(recipientPeerId, packet);
    } else if (m_relayTransport && m_relayTransport->isConnected(recipientPeerId)) {
        m_relayTransport->sendPacket(recipientPeerId, packet);
    }
}

void MessagingEngine::onPeerDiscovered(const discovery::DiscoveredPeer& peer) {
    if (m_store) {
        storage::Contact c;
        c.peerId = peer.peerId;
        c.displayName = peer.displayName;
        c.lastSeen = peer.lastSeen;
        m_store->upsertContact(c);
    }

    emit contactDiscovered(peer.peerId, peer.displayName, peer.hostAddress.toString(), peer.port);

    // Opportunistically establish direct connection if not already connected
    if (m_directTransport && !m_directTransport->isConnected(peer.peerId)) {
        m_directTransport->connectToPeer(peer.peerId, peer.hostAddress.toString(), peer.port);
    }
}

void MessagingEngine::onTransportPeerConnected(const QString& peerId, networking::RouteType route) {
    emit peerRouteChanged(peerId, networking::routeTypeToString(route));
}

void MessagingEngine::onTransportRouteChanged(const QString& peerId, networking::RouteType route) {
    emit peerRouteChanged(peerId, networking::routeTypeToString(route));
}

}  // namespace peerchat::messaging
