// models/CartSummary.cpp — see CartSummary.hpp.
#include "models/CartSummary.hpp"

namespace blissmont::models {

CartSummary::CartSummary(QObject* parent) : QObject(parent) {}

void CartSummary::update(const blissmont::terminal::v1::CartUpdated& s) {
    subtotal_ = QString::fromStdString(s.subtotal_str());
    orderDiscount_ = QString::fromStdString(s.order_discount_str());
    taxTotal_ = QString::fromStdString(s.tax_total_str());
    total_ = QString::fromStdString(s.total_str());
    amountTendered_ = QString::fromStdString(s.amount_tendered_str());
    balanceDue_ = QString::fromStdString(s.balance_due_str());
    changeDue_ = QString::fromStdString(s.change_due_str());
    customerLabel_ = QString::fromStdString(s.customer_label());
    status_ = QString::fromStdString(s.status());
    emit changed();
}

}  // namespace blissmont::models
