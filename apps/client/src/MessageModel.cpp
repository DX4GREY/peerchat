#include "MessageModel.hpp"

namespace peerchat::client {

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_messages.size());
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return QVariant();
    }

    const MessageItem& item = m_messages.at(index.row());
    switch (role) {
        case IdRole:
            return item.id;
        case SenderIdRole:
            return item.senderId;
        case ContentRole:
            return item.content;
        case MediaPathRole:
            return item.mediaPath;
        case StatusStrRole:
            return item.statusStr;
        case IsOutgoingRole:
            return item.isOutgoing;
        case TimestampRole:
            return item.timestamp;
        case TimeFormattedRole:
            return item.timeFormatted;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[SenderIdRole] = "senderId";
    roles[ContentRole] = "content";
    roles[MediaPathRole] = "mediaPath";
    roles[StatusStrRole] = "statusStr";
    roles[IsOutgoingRole] = "isOutgoing";
    roles[TimestampRole] = "timestamp";
    roles[TimeFormattedRole] = "timeFormatted";
    return roles;
}

void MessageModel::setMessages(const QList<MessageItem>& items) {
    beginResetModel();
    m_messages = items;
    endResetModel();
}

void MessageModel::appendMessage(const MessageItem& item) {
    beginInsertRows(QModelIndex(), static_cast<int>(m_messages.size()),
                    static_cast<int>(m_messages.size()));
    m_messages.append(item);
    endInsertRows();
}

void MessageModel::updateStatus(const QString& messageId, const QString& status) {
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages[i].id == messageId) {
            m_messages[i].statusStr = status;
            emit dataChanged(index(i), index(i), {StatusStrRole});
            return;
        }
    }
}

}  // namespace peerchat::client
