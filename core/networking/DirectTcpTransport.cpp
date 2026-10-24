#include "DirectTcpTransport.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "../crypto/CryptoUtils.hpp"

namespace peerchat::networking {

QString routeTypeToString(RouteType route) {
    switch (route) {
        case RouteType::Direct:
            return QStringLiteral("Direct");
        case RouteType::Relay:
            return QStringLiteral("Relay");
        case RouteType::Connecting:
            return QStringLiteral("Connecting");
        case RouteType::Offline:
        default:
            return QStringLiteral("Offline");
    }
}

RouteType stringToRouteType(const QString& str) {
    if (str == QLatin1String("Direct"))
        return RouteType::Direct;
    if (str == QLatin1String("Relay"))
        return RouteType::Relay;
    if (str == QLatin1String("Connecting"))
        return RouteType::Connecting;
    return RouteType::Offline;
}

DirectTcpTransport::DirectTcpTransport(identity::IIdentityProvider* identityProvider,
                                       QObject* parent)
    : IPeerTransport(parent), m_identityProvider(identityProvider) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &DirectTcpTransport::onNewConnection);
}

DirectTcpTransport::~DirectTcpTransport() {
    stopListening();
}

bool DirectTcpTransport::startListening(quint16 port) {
    if (m_server->isListening()) {
        if (m_server->serverPort() == port) {
            return true;
        }
        m_server->close();
    }

    bool ok = m_server->listen(QHostAddress::Any, port);
    if (ok) {
        emit listeningStarted(m_server->serverPort());
    }
    return ok;
}

void DirectTcpTransport::stopListening() {
    if (m_server->isListening()) {
        m_server->close();
        emit listeningStopped();
    }
    for (auto it = m_peerSockets.begin(); it != m_peerSockets.end(); ++it) {
        it.value()->close();
    }
    m_peerSockets.clear();
    m_socketToPeer.clear();
    m_pendingSockets.clear();
    m_expectedPendingPeerId.clear();
}

quint16 DirectTcpTransport::listeningPort() const {
    return m_server->isListening() ? m_server->serverPort() : 0;
}

void DirectTcpTransport::connectToPeer(const QString& peerId, const QString& host, quint16 port) {
    if (isConnected(peerId)) {
        return;
    }

    emit routeChanged(peerId, RouteType::Connecting);

    auto* rawSocket = new QTcpSocket(this);
    auto* framed = new FramedSocket(rawSocket, this);
    m_pendingSockets.insert(framed);
    m_expectedPendingPeerId[framed] = peerId;

    connect(rawSocket, &QTcpSocket::connected, this, [this, framed]() { sendHandshake(framed); });

    connect(framed, &FramedSocket::frameReceived, this, [this, framed](const QByteArray& payload) {
        if (m_pendingSockets.contains(framed)) {
            processHandshake(framed, payload);
        } else {
            onIncomingFrame(payload);
        }
    });

    connect(framed, &FramedSocket::disconnected, this,
            &DirectTcpTransport::onPeerSocketDisconnected);
    connect(framed, &FramedSocket::errorOccurred, this, [this, framed, peerId](const QString& err) {
        Q_UNUSED(err);
        if (m_pendingSockets.contains(framed)) {
            m_pendingSockets.remove(framed);
            m_expectedPendingPeerId.remove(framed);
            emit routeChanged(peerId, RouteType::Offline);
        }
    });

    rawSocket->connectToHost(host, port);
}

void DirectTcpTransport::disconnectPeer(const QString& peerId) {
    if (m_peerSockets.contains(peerId)) {
        FramedSocket* framed = m_peerSockets.take(peerId);
        m_socketToPeer.remove(framed);
        framed->close();
        framed->deleteLater();
        emit peerDisconnected(peerId);
        emit routeChanged(peerId, RouteType::Offline);
    }
}

RouteType DirectTcpTransport::getRouteType(const QString& peerId) const {
    if (m_peerSockets.contains(peerId)) {
        return RouteType::Direct;
    }
    for (auto it = m_expectedPendingPeerId.begin(); it != m_expectedPendingPeerId.end(); ++it) {
        if (it.value() == peerId) {
            return RouteType::Connecting;
        }
    }
    return RouteType::Offline;
}

bool DirectTcpTransport::isConnected(const QString& peerId) const {
    return m_peerSockets.contains(peerId);
}

bool DirectTcpTransport::sendPacket(const QString& peerId, const QByteArray& packet) {
    if (!m_peerSockets.contains(peerId)) {
        return false;
    }
    return m_peerSockets[peerId]->sendFrame(packet);
}

void DirectTcpTransport::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* rawSocket = m_server->nextPendingConnection();
        auto* framed = new FramedSocket(rawSocket, this);
        m_pendingSockets.insert(framed);

        connect(framed, &FramedSocket::frameReceived, this,
                [this, framed](const QByteArray& payload) {
                    if (m_pendingSockets.contains(framed)) {
                        processHandshake(framed, payload);
                    } else {
                        onIncomingFrame(payload);
                    }
                });

        connect(framed, &FramedSocket::disconnected, this,
                &DirectTcpTransport::onPeerSocketDisconnected);

        // Send our handshake to the newly connected inbound peer
        sendHandshake(framed);
    }
}

void DirectTcpTransport::sendHandshake(FramedSocket* framed) {
    if (!m_identityProvider)
        return;

    QJsonObject obj;
    obj["proto"] = QStringLiteral("peerchat/handshake/1.0");
    obj["peer_id"] = m_identityProvider->peerId();
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    QByteArray nonce = crypto::CryptoUtils::randomBytes(16);
    obj["nonce"] = QString::fromUtf8(nonce.toHex());

    QByteArray dataToSign = obj["peer_id"].toString().toUtf8() + ":" +
                            QByteArray::number(obj["timestamp"].toInteger()) + ":" +
                            obj["nonce"].toString().toUtf8();
    QByteArray sig = m_identityProvider->sign(dataToSign);
    obj["signature"] = QString::fromUtf8(sig.toHex());

    QJsonDocument doc(obj);
    framed->sendFrame(doc.toJson(QJsonDocument::Compact));
}

void DirectTcpTransport::processHandshake(FramedSocket* framed, const QByteArray& payload) {
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        framed->close();
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.value("proto").toString() != QLatin1String("peerchat/handshake/1.0")) {
        framed->close();
        return;
    }

    QString remotePeerId = obj.value("peer_id").toString();
    qint64 timestamp = obj.value("timestamp").toInteger();
    QString nonce = obj.value("nonce").toString();
    QByteArray signature = QByteArray::fromHex(obj.value("signature").toString().toUtf8());

    // Verify signature
    QByteArray dataToVerify =
        remotePeerId.toUtf8() + ":" + QByteArray::number(timestamp) + ":" + nonce.toUtf8();

    if (!m_identityProvider ||
        !m_identityProvider->verifyPeerSignature(remotePeerId, dataToVerify, signature)) {
        framed->close();
        return;
    }

    // If this was an outbound connection expecting a specific peer, verify matching ID
    if (m_expectedPendingPeerId.contains(framed)) {
        QString expected = m_expectedPendingPeerId.value(framed);
        if (!expected.isEmpty() && expected != remotePeerId) {
            framed->close();
            return;
        }
    }

    // Success! Move from pending to active peer sockets
    m_pendingSockets.remove(framed);
    m_expectedPendingPeerId.remove(framed);

    // If already connected, replace with latest verified connection
    if (m_peerSockets.contains(remotePeerId)) {
        FramedSocket* old = m_peerSockets.take(remotePeerId);
        m_socketToPeer.remove(old);
        old->close();
        old->deleteLater();
    }

    m_peerSockets[remotePeerId] = framed;
    m_socketToPeer[framed] = remotePeerId;

    emit peerConnected(remotePeerId, RouteType::Direct);
    emit routeChanged(remotePeerId, RouteType::Direct);
}

void DirectTcpTransport::onIncomingFrame(const QByteArray& payload) {
    auto* framed = qobject_cast<FramedSocket*>(sender());
    if (!framed)
        return;

    if (m_socketToPeer.contains(framed)) {
        QString peerId = m_socketToPeer.value(framed);
        emit packetReceived(peerId, payload);
    }
}

void DirectTcpTransport::onPeerSocketDisconnected() {
    auto* framed = qobject_cast<FramedSocket*>(sender());
    if (!framed)
        return;

    if (m_pendingSockets.contains(framed)) {
        QString expected = m_expectedPendingPeerId.value(framed);
        m_pendingSockets.remove(framed);
        m_expectedPendingPeerId.remove(framed);
        if (!expected.isEmpty()) {
            emit routeChanged(expected, RouteType::Offline);
        }
    } else if (m_socketToPeer.contains(framed)) {
        QString peerId = m_socketToPeer.take(framed);
        m_peerSockets.remove(peerId);
        emit peerDisconnected(peerId);
        emit routeChanged(peerId, RouteType::Offline);
    }

    framed->deleteLater();
}

}  // namespace peerchat::networking
