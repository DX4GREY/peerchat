#pragma once

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

#include <cstdint>

namespace peerchat::networking {

class FramedSocket : public QObject {
    Q_OBJECT
public:
    static constexpr quint32 MAX_FRAME_SIZE = 10 * 1024 * 1024;  // 10 MB limit

    explicit FramedSocket(QTcpSocket* socket, QObject* parent = nullptr);
    ~FramedSocket() override;

    QTcpSocket* socket() const { return m_socket; }

    bool sendFrame(const QByteArray& payload);
    void close();

signals:
    void frameReceived(const QByteArray& payload);
    void disconnected();
    void errorOccurred(const QString& errorString);

private slots:
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError();

private:
    QTcpSocket* m_socket{nullptr};
    QByteArray m_buffer;
    quint32 m_expectedLength{0};
    bool m_hasExpectedLength{false};
};

}  // namespace peerchat::networking
