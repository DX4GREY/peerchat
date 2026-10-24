#include "ContactModel.hpp"

namespace peerchat::client {

ContactModel::ContactModel(QObject* parent) : QAbstractListModel(parent) {}

int ContactModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_contacts.size());
}

QVariant ContactModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_contacts.size()) {
        return QVariant();
    }

    const ContactItem& item = m_contacts.at(index.row());
    switch (role) {
        case PeerIdRole:
            return item.peerId;
        case DisplayNameRole:
            return item.displayName;
        case RouteTypeRole:
            return item.routeType;
        case LastSeenRole:
            return item.lastSeen;
        case LastMessageRole:
            return item.lastMessage;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ContactModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[PeerIdRole] = "peerId";
    roles[DisplayNameRole] = "displayName";
    roles[RouteTypeRole] = "routeType";
    roles[LastSeenRole] = "lastSeen";
    roles[LastMessageRole] = "lastMessage";
    return roles;
}

void ContactModel::setContacts(const QList<ContactItem>& items) {
    beginResetModel();
    m_contacts = items;
    endResetModel();
}

void ContactModel::updateOrAddContact(const ContactItem& item) {
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts[i].peerId == item.peerId) {
            m_contacts[i].displayName = item.displayName;
            if (!item.routeType.isEmpty())
                m_contacts[i].routeType = item.routeType;
            if (item.lastSeen > 0)
                m_contacts[i].lastSeen = item.lastSeen;
            if (!item.lastMessage.isEmpty())
                m_contacts[i].lastMessage = item.lastMessage;
            emit dataChanged(index(i), index(i));
            return;
        }
    }

    beginInsertRows(QModelIndex(), static_cast<int>(m_contacts.size()),
                    static_cast<int>(m_contacts.size()));
    m_contacts.append(item);
    endInsertRows();
}

void ContactModel::updateRoute(const QString& peerId, const QString& route) {
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts[i].peerId == peerId) {
            m_contacts[i].routeType = route;
            emit dataChanged(index(i), index(i), {RouteTypeRole});
            return;
        }
    }
}

}  // namespace peerchat::client
