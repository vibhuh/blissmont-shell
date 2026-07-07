// models/OperatorModel.cpp — see OperatorModel.hpp.
#include "models/OperatorModel.hpp"

namespace blissmont::models {

OperatorModel::OperatorModel(QObject* parent) : QAbstractListModel(parent) {}

int OperatorModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(operators_.size());
}

QVariant OperatorModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= operators_.size()) return {};
    const Operator& o = operators_.at(index.row());
    switch (role) {
        case UserIdRole:      return o.userId;
        case DisplayNameRole: return o.displayName;
        case PosRoleRole:     return o.posRole;
        default:              return {};
    }
}

QHash<int, QByteArray> OperatorModel::roleNames() const {
    return {
        {UserIdRole, "userId"},
        {DisplayNameRole, "displayName"},
        {PosRoleRole, "posRole"},
    };
}

void OperatorModel::reset(const blissmont::terminal::v1::OperatorsList& snapshot) {
    beginResetModel();
    operators_.clear();
    operators_.reserve(snapshot.operators_size());
    for (const auto& o : snapshot.operators()) {
        Operator out;
        out.userId = QString::fromStdString(o.user_id());
        out.displayName = QString::fromStdString(o.display_name());
        out.posRole = QString::fromStdString(o.pos_role());
        operators_.push_back(std::move(out));
    }
    endResetModel();
    emit countChanged();
}

void OperatorModel::clear() {
    if (operators_.isEmpty()) return;
    beginResetModel();
    operators_.clear();
    endResetModel();
    emit countChanged();
}

}  // namespace blissmont::models
