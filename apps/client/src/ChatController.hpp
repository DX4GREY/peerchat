#pragma once

#include <QObject>
#include <QStandardPaths>
#include <QString>

#include "../../core/diagnostics/DiagnosticsService.hpp"
#include "../../core/discovery/LanDiscovery.hpp"
#include "../../core/identity/SodiumIdentityProvider.hpp"
#include "../../core/messaging/MessagingEngine.hpp"
#include "../../core/networking/DirectTcpTransport.hpp"
#include "../../core/networking/RelayTransport.hpp"
#include "../../core/storage/SqliteMessageStore.hpp"
#include "../../core/transfers/FileTransferManager.hpp"
#include "ContactModel.hpp"
#include "MessageModel.hpp"
#include "TransferModel.hpp"

namespace peerchat::client {

class ChatController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString myPeerId READ myPeerId NOTIFY myPeerIdChanged)
    Q_PROPERTY(
        QString myDisplayName READ myDisplayName WRITE setMyDisplayName NOTIFY myDisplayNameChanged)
    Q_PROPERTY(QString activeContactPeerId READ activeContactPeerId NOTIFY activeContactChanged)
    Q_PROPERTY(QString activeContactName READ activeContactName NOTIFY activeContactChanged)
    Q_PROPERTY(QString activeRouteType READ activeRouteType NOTIFY activeRouteChanged)
    Q_PROPERTY(quint16 listenPort READ listenPort NOTIFY listenPortChanged)
    Q_PROPERTY(bool isDarkTheme READ isDarkTheme WRITE setDarkTheme NOTIFY themeChanged)
    Q_PROPERTY(ContactModel* contacts READ contacts CONSTANT)
    Q_PROPERTY(MessageModel* messages READ messages CONSTANT)
    Q_PROPERTY(TransferModel* transfers READ transfers CONSTANT)

public:
    explicit ChatController(QObject* parent = nullptr);
    ~ChatController() override;

    bool initialize(const QString& profile = QString(), quint16 preferredPort = 0,
                    const QString& customStoragePath = QString());

    QString myPeerId() const;
    QString myDisplayName() const;
    void setMyDisplayName(const QString& name);

    QString activeContactPeerId() const;
    QString activeContactName() const;
    QString activeRouteType() const;
    quint16 listenPort() const;

    bool isDarkTheme() const { return m_darkTheme; }
    void setDarkTheme(bool dark);

    ContactModel* contacts() { return m_contactModel; }
    MessageModel* messages() { return m_messageModel; }
    TransferModel* transfers() { return m_transferModel; }

    Q_INVOKABLE void selectContact(const QString& peerId);
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void sendFile(const QString& filePath);
    Q_INVOKABLE void addContact(const QString& peerId, const QString& displayName);
    Q_INVOKABLE void connectDirect(const QString& peerId, const QString& host, quint16 port);
    Q_INVOKABLE void acceptTransfer(const QString& transferId);
    Q_INVOKABLE void cancelTransfer(const QString& transferId);
    Q_INVOKABLE void toggleTheme();
    Q_INVOKABLE void configureRelay(const QString& host, quint16 port, const QString& token);

signals:
    void myPeerIdChanged();
    void myDisplayNameChanged();
    void activeContactChanged();
    void activeRouteChanged();
    void listenPortChanged();
    void themeChanged();
    void newNotification(const QString& title, const QString& message);

private slots:
    void onMessageReceived(const storage::StoredMessage& msg);
    void onMessageDelivered(const QString& msgId, qint64 deliveredAt);
    void onPeerRouteChanged(const QString& peerId, const QString& routeString);
    void onContactDiscovered(const QString& peerId, const QString& displayName, const QString& host,
                             quint16 port);

private:
    identity::SodiumIdentityProvider* m_identity{nullptr};
    storage::SqliteMessageStore* m_store{nullptr};
    networking::DirectTcpTransport* m_directTransport{nullptr};
    networking::RelayTransport* m_relayTransport{nullptr};
    discovery::LanDiscovery* m_discovery{nullptr};
    transfers::FileTransferManager* m_transferMgr{nullptr};
    messaging::MessagingEngine* m_engine{nullptr};
    diagnostics::DiagnosticsService* m_diagnostics{nullptr};

    ContactModel* m_contactModel{nullptr};
    MessageModel* m_messageModel{nullptr};
    TransferModel* m_transferModel{nullptr};

    QString m_activeContactPeerId;
    QString m_activeContactName;
    bool m_darkTheme{true};
};

}  // namespace peerchat::client
