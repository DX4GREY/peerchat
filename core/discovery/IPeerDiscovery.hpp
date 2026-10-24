#pragma once

#include <QHostAddress>
#include <QObject>
#include <QString>

namespace peerchat::discovery {

struct DiscoveredPeer {
    QString peerId;
    QString displayName;
    QHostAddress hostAddress;
    quint16 port{0};
    qint64 lastSeen{0};
};

class IPeerDiscovery : public QObject {
    Q_OBJECT
public:
    explicit IPeerDiscovery(QObject* parent = nullptr) : QObject(parent) {}
    ~IPeerDiscovery() override = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    virtual void broadcastAnnouncement() = 0;

signals:
    void peerDiscovered(const DiscoveredPeer& peer);
    void peerLost(const QString& peerId);
};

}  // namespace peerchat::discovery
