#include "DiagnosticsService.hpp"

#include <QDateTime>

namespace peerchat::diagnostics {

DiagnosticsService::DiagnosticsService(QObject* parent) : QObject(parent) {}

void DiagnosticsService::updatePeerRoute(const QString& peerId, const QString& route) {
    auto& d = m_stats[peerId];
    d.peerId = peerId;
    d.routeType = route;
    if (route != QLatin1String("Offline") && d.connectedSince == 0) {
        d.connectedSince = QDateTime::currentMSecsSinceEpoch();
    } else if (route == QLatin1String("Offline")) {
        d.connectedSince = 0;
    }
    emit diagnosticsUpdated(peerId);
}

void DiagnosticsService::recordBytesSent(const QString& peerId, qint64 bytes) {
    auto& d = m_stats[peerId];
    d.peerId = peerId;
    d.bytesSent += bytes;
    emit diagnosticsUpdated(peerId);
}

void DiagnosticsService::recordBytesReceived(const QString& peerId, qint64 bytes) {
    auto& d = m_stats[peerId];
    d.peerId = peerId;
    d.bytesReceived += bytes;
    emit diagnosticsUpdated(peerId);
}

void DiagnosticsService::recordRtt(const QString& peerId, int rttMs) {
    auto& d = m_stats[peerId];
    d.peerId = peerId;
    d.rttMs = rttMs;
    emit diagnosticsUpdated(peerId);
}

PeerDiagnostics DiagnosticsService::getPeerDiagnostics(const QString& peerId) const {
    return m_stats.value(peerId);
}

QList<PeerDiagnostics> DiagnosticsService::getAllDiagnostics() const {
    return m_stats.values();
}

}  // namespace peerchat::diagnostics
