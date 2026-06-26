// models/HeldCartModel.cpp — see HeldCartModel.hpp.
#include "models/HeldCartModel.hpp"

namespace blissmont::models {

HeldCartModel::HeldCartModel(QObject* parent) : QAbstractListModel(parent) {}

int HeldCartModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(held_.size());
}

QVariant HeldCartModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= held_.size()) return {};
    const Held& h = held_.at(index.row());
    switch (role) {
        case HeldCartIdRole: return h.heldCartId;
        case LabelRole:      return h.label;
        case HeldAtRole:     return h.heldAt;
        case LineCountRole:  return h.lineCount;
        case TotalRole:      return h.total;
        default:             return {};
    }
}

QHash<int, QByteArray> HeldCartModel::roleNames() const {
    return {
        {HeldCartIdRole, "heldCartId"},
        {LabelRole, "label"},
        {HeldAtRole, "heldAt"},
        {LineCountRole, "lineCount"},
        {TotalRole, "total"},
    };
}

void HeldCartModel::reset(const blissmont::terminal::v1::HeldCartsList& snapshot) {
    beginResetModel();
    held_.clear();
    held_.reserve(snapshot.held_carts_size());
    for (const auto& h : snapshot.held_carts()) {
        Held out;
        out.heldCartId = QString::fromStdString(h.held_cart_id());
        out.label = QString::fromStdString(h.label());
        out.heldAt = QString::fromStdString(h.held_at());
        out.lineCount = h.line_count();
        out.total = QString::fromStdString(h.total_str());
        held_.push_back(std::move(out));
    }
    endResetModel();
}

void HeldCartModel::clear() {
    if (held_.isEmpty()) return;
    beginResetModel();
    held_.clear();
    endResetModel();
}

}  // namespace blissmont::models
