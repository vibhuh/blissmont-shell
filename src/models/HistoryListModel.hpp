// models/HistoryListModel.hpp — recalled finalized bills as a QAbstractListModel (UX §10).
//
// Reset wholesale from each HistoryResults snapshot (same discipline as CartLineModel /
// TenderListModel): RecallRecent and SearchByCustomer both return a fresh HistoryResults, so
// the recent-list panel replaces rows rather than patching them. One row per BillSummary —
// note the contract row carries NO tender summary and NO item count (see HISTORY brief §3);
// the panel renders only what is here. QML's HistoryPanel binds the role names below; arrow +
// Enter on a row dispatches reprint/recall by its receiptNo.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class HistoryListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        ReceiptNoRole = Qt::UserRole + 1,
        OrderIdRole,
        StatusRole,
        TotalRole,
        SettledAtRole,
        CustomerLabelRole,
        ProvisionalRole,
    };

    explicit HistoryListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::HistoryResults& snapshot);
    // Drop all rows (e.g. when the panel re-opens or a search is cleared).
    void clear();

private:
    struct Bill {
        QString receiptNo;
        QString orderId;
        QString status;
        QString total;
        QString settledAt;
        QString customerLabel;
        bool provisional = false;
    };
    QList<Bill> bills_;
};

}  // namespace blissmont::models
