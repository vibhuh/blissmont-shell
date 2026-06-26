// models/ReturnLineModel.hpp — the original bill's returnable lines as a QAbstractListModel.
//
// Reset wholesale from each ReturnContextLoaded snapshot — same discipline as CartLineModel.
// The engine is the source of truth for the return context: SetReturnLineQty re-emits a fresh
// ReturnContextLoaded with the updated selected qty / restock, so this model is REPLACED on
// every edit, never patched locally. QML's ReturnPanel binds the role names below; a row edit
// dispatches SetReturnLineQty{originalLineNo, qty, restock} and waits for the re-emitted snapshot.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class ReturnLineModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        OriginalLineNoRole = Qt::UserRole + 1,
        ItemIdRole,
        DescriptionRole,
        SoldQtyRole,
        RefundableQtyRole,
        SelectedQtyRole,
        UnitPriceRole,
        LineTotalRole,
        TaxAmountRole,
        RestockRole,
    };

    explicit ReturnLineModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::ReturnContextLoaded& snapshot);
    // Drop all rows (e.g. after a committed return clears the context).
    void clear();

private:
    struct Line {
        int lineNo = 0;
        QString itemId;
        QString description;
        QString soldQty;
        QString refundableQty;
        QString selectedQty;
        QString unitPrice;
        QString lineTotal;
        QString taxAmount;
        bool restock = false;
    };
    QList<Line> lines_;
};

}  // namespace blissmont::models
