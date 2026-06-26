// models/BillDetailModel.cpp — see BillDetailModel.hpp.
#include "models/BillDetailModel.hpp"

namespace blissmont::models {

BillDetailModel::BillDetailModel(QObject* parent)
    : QObject(parent),
      lines_(new CartLineModel(this)),
      payments_(new TenderListModel(this)) {}

void BillDetailModel::reset(const blissmont::terminal::v1::BillDetail& detail) {
    receiptNo_ = QString::fromStdString(detail.receipt_no());
    orderId_ = QString::fromStdString(detail.order_id());
    status_ = QString::fromStdString(detail.status());
    subtotal_ = QString::fromStdString(detail.subtotal_str());
    orderDiscount_ = QString::fromStdString(detail.order_discount_str());
    taxTotal_ = QString::fromStdString(detail.tax_total_str());
    total_ = QString::fromStdString(detail.total_str());
    settledAt_ = QString::fromStdString(detail.settled_at());
    customerLabel_ = QString::fromStdString(detail.customer_label());
    provisional_ = detail.provisional();
    lines_->reset(detail.lines());
    payments_->reset(detail.payments());
    active_ = true;
    emit changed();
}

void BillDetailModel::clear() {
    if (!active_) return;  // already empty — no spurious notify
    receiptNo_.clear();
    orderId_.clear();
    status_.clear();
    subtotal_.clear();
    orderDiscount_.clear();
    taxTotal_.clear();
    total_.clear();
    settledAt_.clear();
    customerLabel_.clear();
    provisional_ = false;
    lines_->reset(google::protobuf::RepeatedPtrField<blissmont::terminal::v1::CartLine>{});
    payments_->reset(google::protobuf::RepeatedPtrField<blissmont::terminal::v1::Tender>{});
    active_ = false;
    emit changed();
}

}  // namespace blissmont::models
