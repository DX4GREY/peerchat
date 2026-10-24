#pragma once

#include "IPeerDiscovery.hpp"
#include "../identity/IIdentityProvider.hpp"

#include <QUdpSocket>
#include <QTimer>
#include <QMap>

namespace peerchat::discovery {

class LanDiscovery : public IPeerDiscovery {
    Q_OBJECT
public:
    static constexpr quint16 DEFAULT_DISCOVERY_PORT = 4242;
    static const QString MULTICAST_GROUP_IPV4;

    explicit LanDiscovery(identity::IIdentityProvider* identityProvider, quint16 tcpListenPort, QObject* parent = nullptr);
    ~LanDiscovery() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void broadcastAnnouncement() override;
    void setTcpListenPort(quint16 port);

private slots:
    void onReadyRead();
    void onBeaconTimeout();
    void onPruneTimeout();

private:
    identity::IIdentityProvider* m_identityProvider{nullptr};
    quint16 m_tcpListenPort{0};
    QUdpSocket* m_udpSocket{nullptr};
    QTimer* m_beaconTimer{nullptr};
    QTimer* m_pruneTimer{nullptr};

    QMap<QString, DiscoveredPeer> m_knownPeers;
    bool m_running{false};
};

}  // namespace peerchat::discovery
