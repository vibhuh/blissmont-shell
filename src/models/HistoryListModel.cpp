// models/HistoryListModel.cpp — see HistoryListModel.hpp.
#include "models/HistoryListModel.hpp"

namespace blissmont::models {

HistoryListModel::HistoryListModel(QObject* parent) : QAbstractListModel(parent) {}

int HistoryListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(bills_.size());
}

QVariant HistoryListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= bills_.size()) return {};
    const Bill& b = bills_.at(index.row());
    switch (role) {
        case ReceiptNoRole:     return b.receiptNo;
        case OrderIdRole:       return b.orderId;
        case StatusRole:        return b.status;
        case TotalRole:         return b.total;
        case SettledAtRole:     return b.settledAt;
        case CustomerLabelRole: return b.customerLabel;
        case ProvisionalRole:   return b.provisional;
        default:                return {};
    }
}

QHash<int, QByteArray> HistoryListModel::roleNames() const {
    return {
        {ReceiptNoRole, "receiptNo"},
        {OrderIdRole, "orderId"},
        {StatusRole, "status"},
        {TotalRole, "total"},
        {SettledAtRole, "settledAt"},
        {CustomerLabelRole, "customerLabel"},
        {ProvisionalRole, "provisional"},
    };
}

void HistoryListModel::reset(const blissmont::terminal::v1::HistoryResults& snapshot) {
    beginResetModel();
    bills_.clear();
    bills_.reserve(snapshot.bills_size());
    for (const auto& b : snapshot.bills()) {
        Bill out;
        out.receiptNo = QString::fromStdString(b.receipt_no());
        out.orderId = QString::fromStdString(b.order_id());
        out.status = QString::fromStdString(b.status());
        out.total = QString::fromStdString(b.total_str());
        out.settledAt = QString::fromStdString(b.settled_at());
        out.customerLabel = QString::fromStdString(b.customer_label());
        out.provisional = b.provisional();
        bills_.push_back(std::move(out));
    }
    endResetModel();
}

void HistoryListModel::clear() {
    if (bills_.isEmpty()) return;
    beginResetModel();
    bills_.clear();
    endResetModel();
}

}  // namespace blissmont::models
