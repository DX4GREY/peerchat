#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include "../../core/storage/IMessageStore.hpp"

namespace peerchat::client {

struct MessageItem {
    QString id;
    QString senderId;
    QString content;
    QString mediaPath;
    QString statusStr;
    bool isOutgoing{false};
    qint64 timestamp{0};
    QString timeFormatted;
};

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SenderIdRole,
        ContentRole,
        MediaPathRole,
        StatusStrRole,
        IsOutgoingRole,
        TimestampRole,
        TimeFormattedRole
    };

    explicit MessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(const QList<MessageItem>& items);
    void appendMessage(const MessageItem& item);
    void updateStatus(const QString& messageId, const QString& status);

private:
    QList<MessageItem> m_messages;
};

}  // namespace peerchat::client
