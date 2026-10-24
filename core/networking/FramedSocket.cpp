#include "FramedSocket.hpp"

#include <QtEndian>

namespace peerchat::networking {

FramedSocket::FramedSocket(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket) {
    if (m_socket) {
        m_socket->setParent(this);
        connect(m_socket, &QTcpSocket::readyRead, this, &FramedSocket::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &FramedSocket::onSocketDisconnected);
        connect(m_socket, &QTcpSocket::errorOccurred, this, &FramedSocket::onSocketError);
    }
}

FramedSocket::~FramedSocket() {
    close();
}

void FramedSocket::close() {
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnect(this);
        m_socket->close();
    }
}

bool FramedSocket::sendFrame(const QByteArray& payload) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    if (payload.size() > static_cast<qsizetype>(MAX_FRAME_SIZE)) {
        return false;
    }

    quint32 length = qToBigEndian(static_cast<quint32>(payload.size()));
    qint64 writtenHeader = m_socket->write(reinterpret_cast<const char*>(&length), sizeof(length));
    qint64 writtenPayload = m_socket->write(payload);

    return (writtenHeader == sizeof(length) && writtenPayload == payload.size());
}

void FramedSocket::onReadyRead() {
    if (!m_socket) return;
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (!m_hasExpectedLength) {
            if (m_buffer.size() < static_cast<qsizetype>(sizeof(quint32))) {
                break;
            }
            quint32 netLength = 0;
            memcpy(&netLength, m_buffer.constData(), sizeof(quint32));
            m_expectedLength = qFromBigEndian(netLength);
            m_hasExpectedLength = true;
            m_buffer.remove(0, sizeof(quint32));

            if (m_expectedLength > MAX_FRAME_SIZE) {
                emit errorOccurred(QStringLiteral("Received frame length exceeds maximum allowed limit"));
                close();
                return;
            }
        }

        if (m_hasExpectedLength) {
            if (m_buffer.size() < static_cast<qsizetype>(m_expectedLength)) {
                break;
            }

            QByteArray frame = m_buffer.left(static_cast<qsizetype>(m_expectedLength));
            m_buffer.remove(0, static_cast<qsizetype>(m_expectedLength));
            m_hasExpectedLength = false;
            m_expectedLength = 0;

            emit frameReceived(frame);
        }
    }
}

void FramedSocket::onSocketDisconnected() {
    emit disconnected();
}

void FramedSocket::onSocketError() {
    if (m_socket) {
        emit errorOccurred(m_socket->errorString());
    }
}

}  // namespace peerchat::networking
