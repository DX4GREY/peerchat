#include "RelayServer.hpp"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>

#include "../../core/crypto/CryptoUtils.hpp"
#include "../../core/identity/IIdentityProvider.hpp"

namespace peerchat::relay {

RelayServer::RelayServer(quint16 port, const QString& authToken, int maxClients,
                         int maxPacketsPerSec, QObject* parent)
    : QObject(parent),
      m_port(port),
      m_authToken(authToken),
      m_maxClients(maxClients),
      m_maxPacketsPerSec(maxPacketsPerSec) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &RelayServer::onNewConnection);
}

RelayServer::~RelayServer() {
    stop();
}

bool RelayServer::start() {
    return m_server->listen(QHostAddress::Any, m_port);
}

void RelayServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
    }
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        it.value().socket->close();
    }
    m_sessions.clear();
    m_socketMap.clear();
}

quint16 RelayServer::port() const {
    return m_server->isListening() ? m_server->serverPort() : 0;
}

int RelayServer::connectedClientCount() const {
    return static_cast<int>(m_sessions.size());
}

void RelayServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* raw = m_server->nextPendingConnection();

        if (m_sessions.size() >= m_maxClients) {
            raw->disconnectFromHost();
            continue;
        }

        auto* framed = new networking::FramedSocket(raw, this);
        connect(framed, &networking::FramedSocket::frameReceived, this,
                &RelayServer::onFrameReceived);
        connect(framed, &networking::FramedSocket::disconnected, this,
                &RelayServer::onSocketDisconnected);
    }
}

void RelayServer::onFrameReceived(const QByteArray& frame) {
    auto* framed = qobject_cast<networking::FramedSocket*>(sender());
    if (!framed)
        return;

    QJsonDocument doc = QJsonDocument::fromJson(frame);
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    if (obj.value("proto").toString() != QLatin1String("peerchat/relay/1.0")) {
        return;
    }

    QString action = obj.value("action").toString();
    if (action == QLatin1String("register")) {
        handleRegister(framed, obj);
    } else if (action == QLatin1String("forward")) {
        handleForward(framed, obj);
    }
}

void RelayServer::handleRegister(networking::FramedSocket* framed, const QJsonObject& obj) {
    QString peerId = obj.value("peer_id").toString();
    QString token = obj.value("auth_token").toString();

    // Check optional token
    if (!m_authToken.isEmpty() && token != m_authToken) {
        framed->close();
        return;
    }

    // Authenticate signature if provided
    qint64 timestamp = obj.value("timestamp").toInteger();
    QByteArray signature = QByteArray::fromHex(obj.value("signature").toString().toUtf8());
    QByteArray dataToVerify =
        peerId.toUtf8() + ":" + QByteArray::number(timestamp) + ":" + token.toUtf8();
    QByteArray pubKey = identity::IIdentityProvider::extractPublicKey(peerId);

    if (pubKey.isEmpty() || !crypto::CryptoUtils::verify(dataToVerify, signature, pubKey)) {
        framed->close();
        return;
    }

    // Register session
    if (m_sessions.contains(peerId)) {
        m_sessions[peerId].socket->close();
        m_socketMap.remove(m_sessions[peerId].socket);
    }

    ClientSession session;
    session.peerId = peerId;
    session.socket = framed;
    session.connectedAt = QDateTime::currentMSecsSinceEpoch();
    session.packetsSentInCurrentSec = 0;
    session.currentSecBucket = session.connectedAt / 1000;

    m_sessions[peerId] = session;
    m_socketMap[framed] = peerId;

    // Send confirmation
    QJsonObject resp;
    resp["proto"] = QStringLiteral("peerchat/relay/1.0");
    resp["action"] = QStringLiteral("registered");
    resp["peer_id"] = peerId;
    QJsonDocument doc(resp);
    framed->sendFrame(doc.toJson(QJsonDocument::Compact));
}

void RelayServer::handleForward(networking::FramedSocket* framed, const QJsonObject& obj) {
    if (!m_socketMap.contains(framed)) {
        return;  // Unauthenticated
    }

    QString senderPeerId = m_socketMap.value(framed);
    auto& session = m_sessions[senderPeerId];

    if (!checkRateLimit(session)) {
        return;  // Dropped by rate limiter
    }

    QString targetPeerId = obj.value("target_peer_id").toString();
    QString b64Payload = obj.value("payload").toString();

    if (!m_sessions.contains(targetPeerId)) {
        return;  // Target not connected to this relay
    }

    auto& targetSession = m_sessions[targetPeerId];
    QJsonObject relayed;
    relayed["proto"] = QStringLiteral("peerchat/relay/1.0");
    relayed["action"] = QStringLiteral("relay_packet");
    relayed["from_peer_id"] = senderPeerId;
    relayed["payload"] = b64Payload;

    QJsonDocument doc(relayed);
    targetSession.socket->sendFrame(doc.toJson(QJsonDocument::Compact));
}

bool RelayServer::checkRateLimit(ClientSession& session) {
    qint64 nowSec = QDateTime::currentMSecsSinceEpoch() / 1000;
    if (nowSec != session.currentSecBucket) {
        session.currentSecBucket = nowSec;
        session.packetsSentInCurrentSec = 0;
    }

    if (session.packetsSentInCurrentSec >= m_maxPacketsPerSec) {
        return false;
    }

    session.packetsSentInCurrentSec++;
    return true;
}

void RelayServer::onSocketDisconnected() {
    auto* framed = qobject_cast<networking::FramedSocket*>(sender());
    if (!framed)
        return;

    if (m_socketMap.contains(framed)) {
        QString peerId = m_socketMap.take(framed);
        m_sessions.remove(peerId);
    }
    framed->deleteLater();
}

}  // namespace peerchat::relay
