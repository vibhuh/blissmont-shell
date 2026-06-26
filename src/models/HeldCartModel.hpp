// models/HeldCartModel.hpp — active held carts (suspend/resume drafts) as a QAbstractListModel (UX §10).
//
// Reset wholesale from each HeldCartsList snapshot (same discipline as HistoryListModel /
// CartLineModel): ListHeldCarts returns a fresh HeldCartsList, so the resume panel replaces
// rows rather than patching them. One row per HeldCartSummary — the contract carries the
// parked draft's id, optional label, when it was held, its line count, and total. QML's
// SuspendPanel binds the role names below; arrow + Enter (or typing the hold number) resumes
// by heldCartId.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class HeldCartModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        HeldCartIdRole = Qt::UserRole + 1,
        LabelRole,
        HeldAtRole,
        LineCountRole,
        TotalRole,
    };

    explicit HeldCartModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::HeldCartsList& snapshot);
    // Drop all rows (e.g. when the panel re-opens).
    void clear();

private:
    struct Held {
        QString heldCartId;
        QString label;
        QString heldAt;
        int lineCount = 0;
        QString total;
    };
    QList<Held> held_;
};

}  // namespace blissmont::models
