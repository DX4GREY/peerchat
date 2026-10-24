#include "RelayTransport.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace peerchat::networking {

RelayTransport::RelayTransport(identity::IIdentityProvider* identityProvider, QObject* parent)
    : IPeerTransport(parent), m_identityProvider(identityProvider) {
    m_rawSocket = new QTcpSocket(this);
    m_framed = new FramedSocket(m_rawSocket, this);

    connect(m_rawSocket, &QTcpSocket::connected, this, &RelayTransport::onRelayConnected);
    connect(m_rawSocket, &QTcpSocket::disconnected, this, &RelayTransport::onRelayDisconnected);
    connect(m_framed, &FramedSocket::frameReceived, this, &RelayTransport::onRelayFrameReceived);
}

RelayTransport::~RelayTransport() {
    stopListening();
}

void RelayTransport::setRelayServer(const QString& host, quint16 port, const QString& authToken) {
    m_relayHost = host;
    m_relayPort = port;
    m_authToken = authToken;

    if (!m_relayHost.isEmpty() && m_relayPort > 0) {
        if (m_rawSocket->state() == QAbstractSocket::ConnectedState) {
            m_rawSocket->disconnectFromHost();
        }
        m_rawSocket->connectToHost(m_relayHost, m_relayPort);
    }
}

bool RelayTransport::isRelayConnected() const {
    return m_isRelayRegistered && (m_rawSocket->state() == QAbstractSocket::ConnectedState);
}

bool RelayTransport::startListening(quint16 port) {
    Q_UNUSED(port);
    // For relay transport, listening means connecting to the configured relay daemon
    if (!m_relayHost.isEmpty() && m_relayPort > 0) {
        if (m_rawSocket->state() == QAbstractSocket::UnconnectedState) {
            m_rawSocket->connectToHost(m_relayHost, m_relayPort);
        }
        return true;
    }
    return false;
}

void RelayTransport::stopListening() {
    m_rawSocket->disconnectFromHost();
    m_isRelayRegistered = false;
    m_activeRelayedPeers.clear();
}

quint16 RelayTransport::listeningPort() const {
    return 0;  // Relay client has no local listening port
}

void RelayTransport::connectToPeer(const QString& peerId, const QString& host, quint16 port) {
    Q_UNUSED(host);
    Q_UNUSED(port);

    if (!isRelayConnected()) {
        emit routeChanged(peerId, RouteType::Offline);
        return;
    }

    m_activeRelayedPeers.insert(peerId);
    emit peerConnected(peerId, RouteType::Relay);
    emit routeChanged(peerId, RouteType::Relay);
}

void RelayTransport::disconnectPeer(const QString& peerId) {
    if (m_activeRelayedPeers.remove(peerId)) {
        emit peerDisconnected(peerId);
        emit routeChanged(peerId, RouteType::Offline);
    }
}

RouteType RelayTransport::getRouteType(const QString& peerId) const {
    if (m_activeRelayedPeers.contains(peerId) && isRelayConnected()) {
        return RouteType::Relay;
    }
    return RouteType::Offline;
}

bool RelayTransport::isConnected(const QString& peerId) const {
    return m_activeRelayedPeers.contains(peerId) && isRelayConnected();
}

bool RelayTransport::sendPacket(const QString& peerId, const QByteArray& packet) {
    if (!isRelayConnected() || !m_activeRelayedPeers.contains(peerId)) {
        return false;
    }

    QJsonObject obj;
    obj["proto"] = QStringLiteral("peerchat/relay/1.0");
    obj["action"] = QStringLiteral("forward");
    obj["target_peer_id"] = peerId;
    obj["payload"] = QString::fromUtf8(packet.toBase64());

    QJsonDocument doc(obj);
    return m_framed->sendFrame(doc.toJson(QJsonDocument::Compact));
}

void RelayTransport::onRelayConnected() {
    registerWithRelay();
}

void RelayTransport::onRelayDisconnected() {
    m_isRelayRegistered = false;
    emit relayConnectionStateChanged(false);

    for (const QString& peerId : m_activeRelayedPeers) {
        emit peerDisconnected(peerId);
        emit routeChanged(peerId, RouteType::Offline);
    }
    m_activeRelayedPeers.clear();
}

void RelayTransport::registerWithRelay() {
    if (!m_identityProvider)
        return;

    QJsonObject obj;
    obj["proto"] = QStringLiteral("peerchat/relay/1.0");
    obj["action"] = QStringLiteral("register");
    obj["peer_id"] = m_identityProvider->peerId();
    obj["auth_token"] = m_authToken;
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray dataToSign = obj["peer_id"].toString().toUtf8() + ":" +
                            QByteArray::number(obj["timestamp"].toInteger()) + ":" +
                            m_authToken.toUtf8();
    obj["signature"] = QString::fromUtf8(m_identityProvider->sign(dataToSign).toHex());

    QJsonDocument doc(obj);
    m_framed->sendFrame(doc.toJson(QJsonDocument::Compact));
}

void RelayTransport::onRelayFrameReceived(const QByteArray& frame) {
    QJsonDocument doc = QJsonDocument::fromJson(frame);
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QString action = obj.value("action").toString();

    if (action == QLatin1String("registered")) {
        m_isRelayRegistered = true;
        emit relayConnectionStateChanged(true);
    } else if (action == QLatin1String("relay_packet")) {
        QString fromPeerId = obj.value("from_peer_id").toString();
        QByteArray payload = QByteArray::fromBase64(obj.value("payload").toString().toUtf8());

        if (!m_activeRelayedPeers.contains(fromPeerId)) {
            m_activeRelayedPeers.insert(fromPeerId);
            emit peerConnected(fromPeerId, RouteType::Relay);
            emit routeChanged(fromPeerId, RouteType::Relay);
        }

        emit packetReceived(fromPeerId, payload);
    }
}

}  // namespace peerchat::networking
