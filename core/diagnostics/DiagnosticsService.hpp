#pragma once

#include <QMap>
#include <QObject>
#include <QString>

namespace peerchat::diagnostics {

struct PeerDiagnostics {
    QString peerId;
    QString routeType;  // "Direct", "Relay", "Connecting", "Offline"
    qint64 bytesSent{0};
    qint64 bytesReceived{0};
    qint64 connectedSince{0};
    int rttMs{0};
};

class DiagnosticsService : public QObject {
    Q_OBJECT
public:
    explicit DiagnosticsService(QObject* parent = nullptr);
    ~DiagnosticsService() override = default;

    void updatePeerRoute(const QString& peerId, const QString& route);
    void recordBytesSent(const QString& peerId, qint64 bytes);
    void recordBytesReceived(const QString& peerId, qint64 bytes);
    void recordRtt(const QString& peerId, int rttMs);

    PeerDiagnostics getPeerDiagnostics(const QString& peerId) const;
    QList<PeerDiagnostics> getAllDiagnostics() const;

signals:
    void diagnosticsUpdated(const QString& peerId);

private:
    QMap<QString, PeerDiagnostics> m_stats;
};

}  // namespace peerchat::diagnostics
