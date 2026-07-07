// models/OperatorModel.hpp — the enabled cashiers for the Begin-Day operator picker
// as a QAbstractListModel (Slice B).
//
// Reset wholesale from each OperatorsList snapshot (same discipline as HeldCartModel /
// HistoryListModel): ListOperators returns a fresh OperatorsList, so the picker
// replaces rows rather than patching them. One row per Operator — the contract carries
// only the real users.id, a display name, and the POS role. The pin_hash NEVER crosses
// to the shell; the engine holds it for offline verification. The Begin-Day screen
// binds the role names below; selecting a row yields the userId that becomes
// OpenShift.cashier_user_id once the PIN checks.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class OperatorModel : public QAbstractListModel {
    Q_OBJECT
    // Exposed so QML can gate the Begin-Day "commissioned" state on the operator count
    // (a bare QAbstractListModel has no `count` in QML). Emits countChanged on reset/clear.
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Role {
        UserIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        PosRoleRole,
    };

    explicit OperatorModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int count() const { return static_cast<int>(operators_.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::OperatorsList& snapshot);
    // Drop all rows (e.g. when the workflow re-opens).
    void clear();

signals:
    void countChanged();

private:
    struct Operator {
        QString userId;
        QString displayName;
        QString posRole;
    };
    QList<Operator> operators_;
};

}  // namespace blissmont::models
