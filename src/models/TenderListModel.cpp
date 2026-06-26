// models/TenderListModel.cpp — see TenderListModel.hpp.
#include "models/TenderListModel.hpp"

namespace blissmont::models {

TenderListModel::TenderListModel(QObject* parent) : QAbstractListModel(parent) {}

int TenderListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(tenders_.size());
}

QVariant TenderListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= tenders_.size()) return {};
    const Tender& t = tenders_.at(index.row());
    switch (role) {
        case TenderNoRole:  return t.tenderNo;
        case MethodRole:    return t.method;
        case AmountRole:    return t.amount;
        case ReferenceRole: return t.reference;
        default:            return {};
    }
}

QHash<int, QByteArray> TenderListModel::roleNames() const {
    return {
        {TenderNoRole, "tenderNo"},
        {MethodRole, "method"},
        {AmountRole, "amount"},
        {ReferenceRole, "reference"},
    };
}

void TenderListModel::reset(const blissmont::terminal::v1::CartUpdated& snapshot) {
    beginResetModel();
    tenders_.clear();
    tenders_.reserve(snapshot.tenders_size());
    for (const auto& t : snapshot.tenders()) {
        Tender out;
        out.tenderNo = t.tender_no();
        out.method = QString::fromStdString(t.method());
        out.amount = QString::fromStdString(t.amount_str());
        out.reference = QString::fromStdString(t.reference());
        tenders_.push_back(std::move(out));
    }
    endResetModel();
}

}  // namespace blissmont::models
