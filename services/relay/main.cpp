#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include "RelayServer.hpp"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("peerchat-relay"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("PeerChat Autonomous Relay & Fallback Server"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  QStringLiteral("Listening port (default: 42424)"),
                                  QStringLiteral("port"),
                                  QStringLiteral("42424"));
    parser.addOption(portOption);

    QCommandLineOption tokenOption(QStringList() << "t" << "auth-token",
                                   QStringLiteral("Shared authentication token"),
                                   QStringLiteral("token"));
    parser.addOption(tokenOption);

    QCommandLineOption maxClientsOption(QStringList() << "m" << "max-clients",
                                        QStringLiteral("Maximum concurrent peers (default: 500)"),
                                        QStringLiteral("max"),
                                        QStringLiteral("500"));
    parser.addOption(maxClientsOption);

    parser.process(app);

    quint16 port = static_cast<quint16>(parser.value(portOption).toUShort());
    QString token = parser.value(tokenOption);
    int maxClients = parser.value(maxClientsOption).toInt();

    peerchat::relay::RelayServer relay(port, token, maxClients);
    if (!relay.start()) {
        QTextStream(stderr) << "Failed to start relay server on port " << port << "\n";
        return 1;
    }

    QTextStream(stdout) << "PeerChat Relay Server started on port " << relay.port()
                        << " (max clients: " << maxClients << ")\n";

    return app.exec();
}
