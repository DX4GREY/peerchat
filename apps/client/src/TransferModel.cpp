#include "TransferModel.hpp"

namespace peerchat::client {

TransferModel::TransferModel(QObject* parent) : QAbstractListModel(parent) {}

int TransferModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_transfers.size());
}

QVariant TransferModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_transfers.size()) {
        return QVariant();
    }

    const TransferListItem& item = m_transfers.at(index.row());
    switch (role) {
        case TransferIdRole: return item.transferId;
        case FileNameRole: return item.fileName;
        case FilePathRole: return item.filePath;
        case ProgressRole: return item.progress;
        case StatusRole: return item.status;
        case IsOutgoingRole: return item.isOutgoing;
        case SizeStrRole: return item.sizeStr;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TransferModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TransferIdRole] = "transferId";
    roles[FileNameRole] = "fileName";
    roles[FilePathRole] = "filePath";
    roles[ProgressRole] = "progress";
    roles[StatusRole] = "status";
    roles[IsOutgoingRole] = "isOutgoing";
    roles[SizeStrRole] = "sizeStr";
    return roles;
}

void TransferModel::updateTransfer(const TransferListItem& item) {
    for (int i = 0; i < m_transfers.size(); ++i) {
        if (m_transfers[i].transferId == item.transferId) {
            m_transfers[i] = item;
            emit dataChanged(index(i), index(i));
            return;
        }
    }

    beginInsertRows(QModelIndex(), static_cast<int>(m_transfers.size()), static_cast<int>(m_transfers.size()));
    m_transfers.append(item);
    endInsertRows();
}

}  // namespace peerchat::client
