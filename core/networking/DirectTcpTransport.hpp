#pragma once

#include "IPeerTransport.hpp"
#include "FramedSocket.hpp"
#include "../identity/IIdentityProvider.hpp"

#include <QTcpServer>
#include <QMap>
#include <QSet>

namespace peerchat::networking {

class DirectTcpTransport : public IPeerTransport {
    Q_OBJECT
public:
    explicit DirectTcpTransport(identity::IIdentityProvider* identityProvider, QObject* parent = nullptr);
    ~DirectTcpTransport() override;

    bool startListening(quint16 port = 0) override;
    void stopListening() override;
    quint16 listeningPort() const override;

    void connectToPeer(const QString& peerId, const QString& host, quint16 port) override;
    void disconnectPeer(const QString& peerId) override;

    RouteType getRouteType(const QString& peerId) const override;
    bool isConnected(const QString& peerId) const override;

    bool sendPacket(const QString& peerId, const QByteArray& packet) override;

private slots:
    void onNewConnection();
    void onIncomingFrame(const QByteArray& payload);
    void onPeerSocketDisconnected();

private:
    void sendHandshake(FramedSocket* framed);
    void processHandshake(FramedSocket* framed, const QByteArray& payload);

    identity::IIdentityProvider* m_identityProvider{nullptr};
    QTcpServer* m_server{nullptr};

    // Active verified peers
    QMap<QString, FramedSocket*> m_peerSockets; // peerId -> FramedSocket
    QMap<FramedSocket*, QString> m_socketToPeer;

    // Unverified incoming/outgoing sockets undergoing handshake
    QSet<FramedSocket*> m_pendingSockets;
    QMap<FramedSocket*, QString> m_expectedPendingPeerId;
};

}  // namespace peerchat::networking
