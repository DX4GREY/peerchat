#include "LanDiscovery.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>

namespace peerchat::discovery {

const QString LanDiscovery::MULTICAST_GROUP_IPV4 = QStringLiteral("239.255.42.99");

LanDiscovery::LanDiscovery(identity::IIdentityProvider* identityProvider, quint16 tcpListenPort,
                           QObject* parent)
    : IPeerDiscovery(parent), m_identityProvider(identityProvider), m_tcpListenPort(tcpListenPort) {
    m_udpSocket = new QUdpSocket(this);
    m_beaconTimer = new QTimer(this);
    m_beaconTimer->setInterval(3000);  // Send beacon every 3s
    connect(m_beaconTimer, &QTimer::timeout, this, &LanDiscovery::onBeaconTimeout);

    m_pruneTimer = new QTimer(this);
    m_pruneTimer->setInterval(10000);  // Prune offline peers every 10s
    connect(m_pruneTimer, &QTimer::timeout, this, &LanDiscovery::onPruneTimeout);

    connect(m_udpSocket, &QUdpSocket::readyRead, this, &LanDiscovery::onReadyRead);
}

LanDiscovery::~LanDiscovery() {
    stop();
}

void LanDiscovery::setTcpListenPort(quint16 port) {
    m_tcpListenPort = port;
}

bool LanDiscovery::start() {
    if (m_running)
        return true;

    // Bind with ShareAddress and ReuseAddressHint for multi-process multicast on same machine
    bool ok = m_udpSocket->bind(QHostAddress::AnyIPv4, DEFAULT_DISCOVERY_PORT,
                                QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    if (ok) {
        m_udpSocket->joinMulticastGroup(QHostAddress(MULTICAST_GROUP_IPV4));
        m_running = true;
        m_beaconTimer->start();
        m_pruneTimer->start();
        broadcastAnnouncement();
    }
    return ok;
}

void LanDiscovery::stop() {
    if (!m_running)
        return;

    m_beaconTimer->stop();
    m_pruneTimer->stop();
    m_udpSocket->leaveMulticastGroup(QHostAddress(MULTICAST_GROUP_IPV4));
    m_udpSocket->close();
    m_knownPeers.clear();
    m_running = false;
}

bool LanDiscovery::isRunning() const {
    return m_running;
}

void LanDiscovery::broadcastAnnouncement() {
    if (!m_running || !m_identityProvider || m_tcpListenPort == 0)
        return;

    QJsonObject obj;
    obj["proto"] = QStringLiteral("peerchat/discovery/1.0");
    obj["peer_id"] = m_identityProvider->peerId();
    obj["display_name"] = m_identityProvider->displayName();
    obj["listen_port"] = m_tcpListenPort;
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // Send multicast and local broadcast
    m_udpSocket->writeDatagram(data, QHostAddress(MULTICAST_GROUP_IPV4), DEFAULT_DISCOVERY_PORT);
    m_udpSocket->writeDatagram(data, QHostAddress::Broadcast, DEFAULT_DISCOVERY_PORT);
}

void LanDiscovery::onBeaconTimeout() {
    broadcastAnnouncement();
}

void LanDiscovery::onReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        QByteArray data = datagram.data();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject())
            continue;

        QJsonObject obj = doc.object();
        if (obj.value("proto").toString() != QLatin1String("peerchat/discovery/1.0")) {
            continue;
        }

        QString remotePeerId = obj.value("peer_id").toString();
        // Ignore self announcement
        if (m_identityProvider && remotePeerId == m_identityProvider->peerId()) {
            continue;
        }

        DiscoveredPeer peer;
        peer.peerId = remotePeerId;
        peer.displayName = obj.value("display_name").toString();
        peer.hostAddress = datagram.senderAddress();
        peer.port = static_cast<quint16>(obj.value("listen_port").toInt());
        peer.lastSeen = QDateTime::currentMSecsSinceEpoch();

        // Standardize IPv4 mapped IPv6 address
        if (peer.hostAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            bool ok = false;
            QHostAddress ipv4 = QHostAddress(peer.hostAddress.toIPv4Address(&ok));
            if (ok) {
                peer.hostAddress = ipv4;
            }
        }

        m_knownPeers[remotePeerId] = peer;
        emit peerDiscovered(peer);
    }
}

void LanDiscovery::onPruneTimeout() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = m_knownPeers.begin();
    while (it != m_knownPeers.end()) {
        // If not seen for 25 seconds, mark lost
        if (now - it.value().lastSeen > 25000) {
            QString peerId = it.key();
            it = m_knownPeers.erase(it);
            emit peerLost(peerId);
        } else {
            ++it;
        }
    }
}

}  // namespace peerchat::discovery
