#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace peerchat::networking {

enum class RouteType {
    Direct,
    Relay,
    Connecting,
    Offline
};

QString routeTypeToString(RouteType route);
RouteType stringToRouteType(const QString& str);

class IPeerTransport : public QObject {
    Q_OBJECT
public:
    explicit IPeerTransport(QObject* parent = nullptr) : QObject(parent) {}
    ~IPeerTransport() override = default;

    // Start listening on local interface/port
    virtual bool startListening(quint16 port = 0) = 0;
    virtual void stopListening() = 0;
    virtual quint16 listeningPort() const = 0;

    // Connect to peer endpoint directly or via relay
    virtual void connectToPeer(const QString& peerId, const QString& host, quint16 port) = 0;
    virtual void disconnectPeer(const QString& peerId) = 0;

    // Query active route to peer
    virtual RouteType getRouteType(const QString& peerId) const = 0;
    virtual bool isConnected(const QString& peerId) const = 0;

    // Send raw framed payload to peer
    virtual bool sendPacket(const QString& peerId, const QByteArray& packet) = 0;

signals:
    void peerConnected(const QString& peerId, RouteType route);
    void peerDisconnected(const QString& peerId);
    void packetReceived(const QString& peerId, const QByteArray& packet);
    void routeChanged(const QString& peerId, RouteType newRoute);
    void listeningStarted(quint16 port);
    void listeningStopped();
};

}  // namespace peerchat::networking
