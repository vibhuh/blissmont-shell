// models/BillDetailModel.hpp — the view-only detail of one recalled bill (UX §10).
//
// BillDetail is a header + repeated lines + repeated payments, so it is modelled as a QObject
// with bindable header Q_PROPERTYs plus two nested list models for the body. The body REUSES
// CartLineModel / TenderListModel (BillDetail.lines are CartLine, BillDetail.payments are
// Tender — the exact messages those models already wrap) via their repeated-field reset
// overload, with dedicated instances separate from the bridge's live-cart ones. Reset wholesale
// from each BillDetail; `active` flips with content so the panel toggles list ↔ detail. There
// is NO duplicate flag here — reprint marks DUPLICATE engine-side on the printed bytes, never on
// this message (HISTORY brief §7).
#pragma once

#include <QObject>
#include <QString>

#include "models/CartLineModel.hpp"
#include "models/TenderListModel.hpp"
#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class BillDetailModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(QString receiptNo READ receiptNo NOTIFY changed)
    Q_PROPERTY(QString orderId READ orderId NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString subtotal READ subtotal NOTIFY changed)
    Q_PROPERTY(QString orderDiscount READ orderDiscount NOTIFY changed)
    Q_PROPERTY(QString taxTotal READ taxTotal NOTIFY changed)
    Q_PROPERTY(QString total READ total NOTIFY changed)
    Q_PROPERTY(QString settledAt READ settledAt NOTIFY changed)
    Q_PROPERTY(QString customerLabel READ customerLabel NOTIFY changed)
    Q_PROPERTY(bool provisional READ provisional NOTIFY changed)
    Q_PROPERTY(blissmont::models::CartLineModel* lines READ lines CONSTANT)
    Q_PROPERTY(blissmont::models::TenderListModel* payments READ payments CONSTANT)

public:
    explicit BillDetailModel(QObject* parent = nullptr);

    [[nodiscard]] bool active() const { return active_; }
    [[nodiscard]] QString receiptNo() const { return receiptNo_; }
    [[nodiscard]] QString orderId() const { return orderId_; }
    [[nodiscard]] QString status() const { return status_; }
    [[nodiscard]] QString subtotal() const { return subtotal_; }
    [[nodiscard]] QString orderDiscount() const { return orderDiscount_; }
    [[nodiscard]] QString taxTotal() const { return taxTotal_; }
    [[nodiscard]] QString total() const { return total_; }
    [[nodiscard]] QString settledAt() const { return settledAt_; }
    [[nodiscard]] QString customerLabel() const { return customerLabel_; }
    [[nodiscard]] bool provisional() const { return provisional_; }
    [[nodiscard]] CartLineModel* lines() const { return lines_; }
    [[nodiscard]] TenderListModel* payments() const { return payments_; }

    // Replace the whole detail from a fresh engine snapshot (recall or reprint source).
    void reset(const blissmont::terminal::v1::BillDetail& detail);
    // Drop the detail (back to the recent list / panel re-open).
    void clear();

signals:
    void changed();

private:
    bool active_ = false;
    QString receiptNo_, orderId_, status_;
    QString subtotal_, orderDiscount_, taxTotal_, total_;
    QString settledAt_, customerLabel_;
    bool provisional_ = false;
    CartLineModel* lines_;
    TenderListModel* payments_;
};

}  // namespace blissmont::models
