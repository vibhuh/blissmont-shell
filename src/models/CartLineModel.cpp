// models/CartLineModel.cpp — see CartLineModel.hpp.
#include "models/CartLineModel.hpp"

namespace blissmont::models {

CartLineModel::CartLineModel(QObject* parent) : QAbstractListModel(parent) {}

int CartLineModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(lines_.size());
}

QVariant CartLineModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= lines_.size()) return {};
    const Line& l = lines_.at(index.row());
    switch (role) {
        case LineNoRole:          return l.lineNo;
        case SkuRole:             return l.sku;
        case DescriptionRole:     return l.description;
        case QtyRole:             return l.qty;
        case UnitPriceRole:       return l.unitPrice;
        case PriceOverriddenRole: return l.priceOverridden;
        case DiscountRole:        return l.discount;
        case TaxRateRole:         return l.taxRate;
        case TaxAmountRole:       return l.taxAmount;
        case LineTotalRole:       return l.lineTotal;
        default:                  return {};
    }
}

QHash<int, QByteArray> CartLineModel::roleNames() const {
    return {
        {LineNoRole, "lineNo"},
        {SkuRole, "sku"},
        {DescriptionRole, "description"},
        {QtyRole, "qty"},
        {UnitPriceRole, "unitPrice"},
        {PriceOverriddenRole, "priceOverridden"},
        {DiscountRole, "discount"},
        {TaxRateRole, "taxRate"},
        {TaxAmountRole, "taxAmount"},
        {LineTotalRole, "lineTotal"},
    };
}

void CartLineModel::reset(const blissmont::terminal::v1::CartUpdated& snapshot) {
    reset(snapshot.lines());
}

void CartLineModel::reset(const google::protobuf::RepeatedPtrField<
                          blissmont::terminal::v1::CartLine>& lines) {
    beginResetModel();
    lines_.clear();
    lines_.reserve(lines.size());
    for (const auto& l : lines) {
        Line out;
        out.lineNo = l.line_no();
        out.sku = QString::fromStdString(l.sku());
        out.description = QString::fromStdString(l.description());
        out.qty = QString::fromStdString(l.qty_str());
        out.unitPrice = QString::fromStdString(l.unit_price_str());
        out.priceOverridden = l.price_overridden();
        out.discount = QString::fromStdString(l.discount_str());
        out.taxRate = QString::fromStdString(l.tax_rate_str());
        out.taxAmount = QString::fromStdString(l.tax_amount_str());
        out.lineTotal = QString::fromStdString(l.line_total_str());
        lines_.push_back(std::move(out));
    }
    endResetModel();
}

}  // namespace blissmont::models
