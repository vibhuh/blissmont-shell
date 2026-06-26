// models/ReturnLineModel.cpp — see ReturnLineModel.hpp.
#include "models/ReturnLineModel.hpp"

namespace blissmont::models {

ReturnLineModel::ReturnLineModel(QObject* parent) : QAbstractListModel(parent) {}

int ReturnLineModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(lines_.size());
}

QVariant ReturnLineModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= lines_.size()) return {};
    const Line& l = lines_.at(index.row());
    switch (role) {
        case OriginalLineNoRole: return l.lineNo;
        case ItemIdRole:         return l.itemId;
        case DescriptionRole:    return l.description;
        case SoldQtyRole:        return l.soldQty;
        case RefundableQtyRole:  return l.refundableQty;
        case SelectedQtyRole:    return l.selectedQty;
        case UnitPriceRole:      return l.unitPrice;
        case LineTotalRole:      return l.lineTotal;
        case TaxAmountRole:      return l.taxAmount;
        case RestockRole:        return l.restock;
        default:                 return {};
    }
}

QHash<int, QByteArray> ReturnLineModel::roleNames() const {
    return {
        {OriginalLineNoRole, "originalLineNo"},
        {ItemIdRole, "itemId"},
        {DescriptionRole, "description"},
        {SoldQtyRole, "soldQty"},
        {RefundableQtyRole, "refundableQty"},
        {SelectedQtyRole, "selectedQty"},
        {UnitPriceRole, "unitPrice"},
        {LineTotalRole, "lineTotal"},
        {TaxAmountRole, "taxAmount"},
        {RestockRole, "restock"},
    };
}

void ReturnLineModel::reset(const blissmont::terminal::v1::ReturnContextLoaded& snapshot) {
    beginResetModel();
    lines_.clear();
    lines_.reserve(snapshot.lines_size());
    for (const auto& l : snapshot.lines()) {
        Line out;
        out.lineNo = l.original_line_no();
        out.itemId = QString::fromStdString(l.item_id());
        out.description = QString::fromStdString(l.description());
        out.soldQty = QString::fromStdString(l.sold_qty_str());
        out.refundableQty = QString::fromStdString(l.refundable_qty_str());
        out.selectedQty = QString::fromStdString(l.selected_qty_str());
        out.unitPrice = QString::fromStdString(l.unit_price_str());
        out.lineTotal = QString::fromStdString(l.line_total_str());
        out.taxAmount = QString::fromStdString(l.tax_amount_str());
        out.restock = l.restock();
        lines_.push_back(std::move(out));
    }
    endResetModel();
}

void ReturnLineModel::clear() {
    if (lines_.isEmpty()) return;
    beginResetModel();
    lines_.clear();
    endResetModel();
}

}  // namespace blissmont::models
