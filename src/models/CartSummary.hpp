// models/CartSummary.hpp — bill totals as bindable Q_PROPERTYs (spec §4).
// Updated from each CartUpdated snapshot; QML binds the footer to these. Strings, not
// doubles — amounts are the engine's exact decimal strings, rendered verbatim.
#pragma once

#include <QObject>
#include <QString>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class CartSummary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString subtotal READ subtotal NOTIFY changed)
    Q_PROPERTY(QString orderDiscount READ orderDiscount NOTIFY changed)
    Q_PROPERTY(QString taxTotal READ taxTotal NOTIFY changed)
    Q_PROPERTY(QString total READ total NOTIFY changed)
    Q_PROPERTY(QString amountTendered READ amountTendered NOTIFY changed)
    Q_PROPERTY(QString balanceDue READ balanceDue NOTIFY changed)
    Q_PROPERTY(QString changeDue READ changeDue NOTIFY changed)
    Q_PROPERTY(QString customerLabel READ customerLabel NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)

public:
    explicit CartSummary(QObject* parent = nullptr);

    [[nodiscard]] QString subtotal() const { return subtotal_; }
    [[nodiscard]] QString orderDiscount() const { return orderDiscount_; }
    [[nodiscard]] QString taxTotal() const { return taxTotal_; }
    [[nodiscard]] QString total() const { return total_; }
    [[nodiscard]] QString amountTendered() const { return amountTendered_; }
    [[nodiscard]] QString balanceDue() const { return balanceDue_; }
    [[nodiscard]] QString changeDue() const { return changeDue_; }
    [[nodiscard]] QString customerLabel() const { return customerLabel_; }
    [[nodiscard]] QString status() const { return status_; }

    void update(const blissmont::terminal::v1::CartUpdated& snapshot);

signals:
    void changed();

private:
    QString subtotal_, orderDiscount_, taxTotal_, total_;
    QString amountTendered_, balanceDue_, changeDue_;
    QString customerLabel_, status_;
};

}  // namespace blissmont::models
