// models/CartLineModel.hpp — the bill's line items as a QAbstractListModel (spec §4).
//
// Reset wholesale from each CartUpdated snapshot: the engine pushes a full cart on every
// change (carts are tiny — no deltas), so we never patch rows, we replace them. This makes
// UI desync impossible by construction. QML's BillTable binds to the role names below.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include <google/protobuf/repeated_ptr_field.h>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class CartLineModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        LineNoRole = Qt::UserRole + 1,
        SkuRole,
        DescriptionRole,
        QtyRole,
        UnitPriceRole,
        PriceOverriddenRole,
        DiscountRole,
        TaxRateRole,
        TaxAmountRole,
        LineTotalRole,
    };

    explicit CartLineModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::CartUpdated& snapshot);
    // Replace all rows from a bare repeated CartLine field (e.g. BillDetail.lines for a
    // recalled bill). The CartUpdated overload delegates here — same row shape, two sources.
    void reset(const google::protobuf::RepeatedPtrField<
               blissmont::terminal::v1::CartLine>& lines);

private:
    struct Line {
        int lineNo = 0;
        QString sku;
        QString description;
        QString qty;
        QString unitPrice;
        bool priceOverridden = false;
        QString discount;
        QString taxRate;
        QString taxAmount;
        QString lineTotal;
    };
    QList<Line> lines_;
};

}  // namespace blissmont::models
