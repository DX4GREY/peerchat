#pragma once

#include <QObject>
#include <QSet>

#include "../discovery/IPeerDiscovery.hpp"
#include "../identity/IIdentityProvider.hpp"
#include "../networking/IPeerTransport.hpp"
#include "../storage/IMessageStore.hpp"
#include "../transfers/IFileTransfer.hpp"
#include "ProtocolMessage.hpp"

namespace peerchat::messaging {

class MessagingEngine : public QObject {
    Q_OBJECT
public:
    explicit MessagingEngine(identity::IIdentityProvider* identityProvider,
                             networking::IPeerTransport* directTransport,
                             networking::IPeerTransport* relayTransport,
                             discovery::IPeerDiscovery* discovery, storage::IMessageStore* store,
                             transfers::IFileTransfer* transferManager, QObject* parent = nullptr);
    ~MessagingEngine() override;

    bool initialize(const QString& dataDir, const QString& profileName = QString());

    // Outgoing Operations
    QString sendTextMessage(const QString& recipientPeerId, const QString& text);
    QString sendFile(const QString& recipientPeerId, const QString& filePath);

    // Contacts
    bool addOrUpdateContact(const QString& peerId, const QString& displayName,
                            const QString& trustLevel = QStringLiteral("untrusted"));
    QList<storage::Contact> getContacts() const;

    // Route & Connection query
    networking::RouteType getRoute(const QString& peerId) const;
    void connectToPeerDirect(const QString& peerId, const QString& host, quint16 port);

signals:
    void messageReceived(const storage::StoredMessage& message);
    void messageDelivered(const QString& messageId, qint64 deliveredAt);
    void contactDiscovered(const QString& peerId, const QString& displayName, const QString& host,
                           quint16 port);
    void peerRouteChanged(const QString& peerId, const QString& routeString);

private slots:
    void onPacketReceived(const QString& peerId, const QByteArray& packet);
    void onPeerDiscovered(const discovery::DiscoveredPeer& peer);
    void onTransportPeerConnected(const QString& peerId, networking::RouteType route);
    void onTransportRouteChanged(const QString& peerId, networking::RouteType route);

private:
    void sendDeliveryAck(const QString& recipientPeerId, const QString& ackMessageId);

    identity::IIdentityProvider* m_identityProvider{nullptr};
    networking::IPeerTransport* m_directTransport{nullptr};
    networking::IPeerTransport* m_relayTransport{nullptr};
    discovery::IPeerDiscovery* m_discovery{nullptr};
    storage::IMessageStore* m_store{nullptr};
    transfers::IFileTransfer* m_transferManager{nullptr};

    QSet<QString> m_processedMessageIds;
    QString m_dataDir;
};

}  // namespace peerchat::messaging
