#pragma once

#include <QMap>
#include <QSet>
#include <QTcpSocket>

#include "../identity/IIdentityProvider.hpp"
#include "FramedSocket.hpp"
#include "IPeerTransport.hpp"

namespace peerchat::networking {

class RelayTransport : public IPeerTransport {
    Q_OBJECT
public:
    explicit RelayTransport(identity::IIdentityProvider* identityProvider,
                            QObject* parent = nullptr);
    ~RelayTransport() override;

    bool startListening(quint16 port = 0) override;
    void stopListening() override;
    quint16 listeningPort() const override;

    void connectToPeer(const QString& peerId, const QString& host, quint16 port) override;
    void disconnectPeer(const QString& peerId) override;

    RouteType getRouteType(const QString& peerId) const override;
    bool isConnected(const QString& peerId) const override;

    bool sendPacket(const QString& peerId, const QByteArray& packet) override;

    // Relay Server Configuration
    void setRelayServer(const QString& host, quint16 port, const QString& authToken = QString());
    bool isRelayConnected() const;

signals:
    void relayConnectionStateChanged(bool connected);

private slots:
    void onRelayConnected();
    void onRelayDisconnected();
    void onRelayFrameReceived(const QByteArray& frame);

private:
    void registerWithRelay();

    identity::IIdentityProvider* m_identityProvider{nullptr};
    QString m_relayHost;
    quint16 m_relayPort{0};
    QString m_authToken;

    QTcpSocket* m_rawSocket{nullptr};
    FramedSocket* m_framed{nullptr};
    bool m_isRelayRegistered{false};

    QSet<QString> m_activeRelayedPeers;
};

}  // namespace peerchat::networking
