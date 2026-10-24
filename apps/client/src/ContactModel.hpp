#pragma once

#include <QAbstractListModel>
#include "../../core/storage/IMessageStore.hpp"

namespace peerchat::client {

struct ContactItem {
    QString peerId;
    QString displayName;
    QString routeType{"Offline"}; // "Direct", "Relay", "Connecting", "Offline"
    qint64 lastSeen{0};
    QString lastMessage;
};

class ContactModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PeerIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        RouteTypeRole,
        LastSeenRole,
        LastMessageRole
    };

    explicit ContactModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setContacts(const QList<ContactItem>& items);
    void updateOrAddContact(const ContactItem& item);
    void updateRoute(const QString& peerId, const QString& route);

private:
    QList<ContactItem> m_contacts;
};

}  // namespace peerchat::client
