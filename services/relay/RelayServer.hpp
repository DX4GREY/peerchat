#pragma once

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTcpServer>

#include "../../core/networking/FramedSocket.hpp"

namespace peerchat::relay {

struct ClientSession {
    QString peerId;
    networking::FramedSocket* socket{nullptr};
    qint64 connectedAt{0};
    int packetsSentInCurrentSec{0};
    qint64 currentSecBucket{0};
};

class RelayServer : public QObject {
    Q_OBJECT
public:
    explicit RelayServer(quint16 port = 42424, const QString& authToken = QString(),
                         int maxClients = 500, int maxPacketsPerSec = 50,
                         QObject* parent = nullptr);
    ~RelayServer() override;

    bool start();
    void stop();
    quint16 port() const;
    int connectedClientCount() const;

private slots:
    void onNewConnection();
    void onFrameReceived(const QByteArray& frame);
    void onSocketDisconnected();

private:
    void handleRegister(networking::FramedSocket* framed, const QJsonObject& obj);
    void handleForward(networking::FramedSocket* framed, const QJsonObject& obj);
    bool checkRateLimit(ClientSession& session);

    quint16 m_port;
    QString m_authToken;
    int m_maxClients;
    int m_maxPacketsPerSec;

    QTcpServer* m_server{nullptr};
    QMap<QString, ClientSession> m_sessions;               // peerId -> ClientSession
    QMap<networking::FramedSocket*, QString> m_socketMap;  // FramedSocket -> peerId
};

}  // namespace peerchat::relay
