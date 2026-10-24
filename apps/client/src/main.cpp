#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "ChatController.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PeerChat"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setOrganizationName(QStringLiteral("PeerChat"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("peerchat.local"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("PeerChat - Secure Cross-Platform P2P Messaging"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption profileOption(QStringList() << "p" << "profile",
                                     QStringLiteral("User profile name (e.g. alice, bob)"),
                                     QStringLiteral("name"));
    parser.addOption(profileOption);

    QCommandLineOption portOption(QStringList() << "port", QStringLiteral("TCP listen port"),
                                  QStringLiteral("port"));
    parser.addOption(portOption);

    QCommandLineOption storageOption(QStringList() << "s" << "storage",
                                     QStringLiteral("Custom storage directory path"),
                                     QStringLiteral("dir"));
    parser.addOption(storageOption);

    parser.process(app);

    QString profile = parser.value(profileOption);
    quint16 port = static_cast<quint16>(parser.value(portOption).toUShort());
    QString storageDir = parser.value(storageOption);

    auto* controller = new peerchat::client::ChatController(&app);
    if (!controller->initialize(profile, port, storageDir)) {
        qCritical() << "Failed to initialize PeerChat application engine!";
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("chatController"), controller);

    const QUrl url(QStringLiteral("qrc:/peerchat/qml/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
