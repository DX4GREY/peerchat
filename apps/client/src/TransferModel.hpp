#pragma once

#include <QAbstractListModel>

namespace peerchat::client {

struct TransferListItem {
    QString transferId;
    QString fileName;
    QString filePath;
    qint64 fileSize{0};
    qint64 transferredBytes{0};
    double progress{0.0};
    QString status;
    bool isOutgoing{true};
    QString sizeStr;
};

class TransferModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        TransferIdRole = Qt::UserRole + 1,
        FileNameRole,
        FilePathRole,
        ProgressRole,
        StatusRole,
        IsOutgoingRole,
        SizeStrRole
    };

    explicit TransferModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateTransfer(const TransferListItem& item);

private:
    QList<TransferListItem> m_transfers;
};

}  // namespace peerchat::client
