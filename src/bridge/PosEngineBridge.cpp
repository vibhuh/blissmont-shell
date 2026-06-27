// bridge/PosEngineBridge.cpp — see PosEngineBridge.hpp.
#include "bridge/PosEngineBridge.hpp"

#include <QMetaObject>
#include <QVariantMap>

#include <utility>

namespace blissmont::bridge {

PosEngineBridge::PosEngineBridge(QObject* parent)
    : QObject(parent),
      cart_(new blissmont::models::CartLineModel(this)),
      summary_(new blissmont::models::CartSummary(this)),
      tenders_(new blissmont::models::TenderListModel(this)),
      returnLines_(new blissmont::models::ReturnLineModel(this)),
      history_(new blissmont::models::HistoryListModel(this)),
      billDetail_(new blissmont::models::BillDetailModel(this)),
      heldCarts_(new blissmont::models::HeldCartModel(this)) {}

PosEngineBridge::~PosEngineBridge() { disconnectFromEngine(); }

void PosEngineBridge::setConnected(bool value) {
    if (connected_.exchange(value, std::memory_order_relaxed) != value) {
        // Hop to the UI thread for the notify (we may be on the reader thread).
        QMetaObject::invokeMethod(this, [this] { emit connectionChanged(); },
                                  Qt::QueuedConnection);
    }
}

void PosEngineBridge::connectToEngine(const QString& target) {
    if (stream_) return;  // already connected
    stopping_.store(false, std::memory_order_relaxed);

    channel_ = grpc::CreateChannel(target.toStdString(), grpc::InsecureChannelCredentials());
    stub_ = blissmont::terminal::v1::TerminalEngine::NewStub(channel_);
    ctx_ = std::make_unique<grpc::ClientContext>();
    stream_ = stub_->Session(ctx_.get());

    // One dedicated reader thread (gRPC sync API: exactly one reader, one writer).
    readerThread_.reset(QThread::create([this] { readLoop(); }));
    readerThread_->setObjectName(QStringLiteral("pos-engine-reader"));
    readerThread_->start();
    setConnected(true);
}

void PosEngineBridge::disconnectFromEngine() {
    stopping_.store(true, std::memory_order_relaxed);
    if (stream_) {
        stream_->WritesDone();
        if (ctx_) ctx_->TryCancel();
    }
    if (readerThread_) {
        readerThread_->quit();
        readerThread_->wait();
        readerThread_.reset();
    }
    stream_.reset();
    stub_.reset();
    ctx_.reset();
    channel_.reset();
    setConnected(false);
}

// Reader loop on the worker thread. Each decoded Event is marshaled to the UI thread
// (where `this` lives) via a queued lambda — no protobuf Qt-metatype registration needed.
void PosEngineBridge::readLoop() {
    Event evt;
    while (!stopping_.load(std::memory_order_relaxed) && stream_ && stream_->Read(&evt)) {
        Event copy = evt;
        QMetaObject::invokeMethod(this, [this, copy = std::move(copy)] { applyEvent(copy); },
                                  Qt::QueuedConnection);
        evt.Clear();
    }
    setConnected(false);
}

void PosEngineBridge::writeCommand(Command cmd) {
    if (!stream_) return;
    QMutexLocker lock(&writeMutex_);
    stream_->Write(cmd);
}

// ── UI thread: fold one engine Event into models/signals (spec §4) ────────────
void PosEngineBridge::applyEvent(const Event& evt) {
    using E = blissmont::terminal::v1::Event;
    switch (evt.evt_case()) {
        case E::kCartUpdated:
            cart_->reset(evt.cart_updated());        // full snapshot reset — carts are small
            summary_->update(evt.cart_updated());
            tenders_->reset(evt.cart_updated());     // running tenders ride the same snapshot
            break;
        case E::kOrderSettled:
            emit orderSettled(QString::fromStdString(evt.order_settled().receipt_no()),
                              evt.order_settled().provisional(),
                              QString::fromStdString(evt.order_settled().total_str()));
            break;
        case E::kItemNotFound:
            emit itemNotFound(QString::fromStdString(evt.item_not_found().barcode()));
            break;
        case E::kCommandRejected:
            emit commandRejected(QString::fromStdString(evt.command_rejected().code()),
                                 QString::fromStdString(evt.command_rejected().message()));
            break;
        case E::kShiftStateChanged:
            emit shiftStateChanged(QString::fromStdString(evt.shift_state_changed().shift_id()),
                                   QString::fromStdString(evt.shift_state_changed().status()));
            break;
        case E::kSyncStatusChanged:
            emit syncStatusChanged(evt.sync_status_changed().online(),
                                   evt.sync_status_changed().pending());
            break;
        case E::kConfigUpdated: {
            // Engine-relayed device config (spec §3 gap, closed in contracts v1.1.0).
            // Project the device-domain TerminalConfig onto plain Qt types for the
            // services; the same snapshot lands on connect, reconnect, and change.
            const auto& cfg = evt.config_updated().config();
            // Project the repeated device-domain PaymentMethod into plain Qt rows
            // (contracts v1.2.0). Only the device-surface fields cross — the proto
            // type carries no secrets, so the boundary is enforced by the contract.
            QVariantList paymentMethods;
            paymentMethods.reserve(cfg.payment_methods_size());
            for (const auto& pm : cfg.payment_methods()) {
                QVariantMap row;
                row[QStringLiteral("method")] = QString::fromStdString(pm.method());
                row[QStringLiteral("displayName")] = QString::fromStdString(pm.display_name());
                row[QStringLiteral("hotkey")] = QString::fromStdString(pm.hotkey());
                row[QStringLiteral("sortOrder")] = pm.sort_order();
                row[QStringLiteral("enabled")] = pm.enabled();
                row[QStringLiteral("referenceMode")] = QString::fromStdString(pm.reference_mode());
                paymentMethods.push_back(row);
            }
            // Payout category KEYS only (the category→GL mapping stays server-side).
            QStringList payoutCategories;
            payoutCategories.reserve(cfg.payout_categories_size());
            for (const auto& key : cfg.payout_categories()) {
                payoutCategories.push_back(QString::fromStdString(key));
            }
            emit configUpdated(cfg.allow_returns(), cfg.payout_enabled(),
                               cfg.allow_discounts(),
                               QString::fromStdString(cfg.tender_complete_mode()),
                               QString::fromStdString(cfg.currency_symbol()),
                               paymentMethods,
                               cfg.allow_blind_return(),
                               QString::fromStdString(cfg.refund_tender_mode()),
                               QString::fromStdString(cfg.return_requires_auth()),
                               cfg.restock_default(),
                               cfg.allow_partial_return(),
                               QString::fromStdString(cfg.held_cart_expiry()),
                               payoutCategories);
            break;
        }
        case E::kPayoutRecorded:
            // The engine echoes the recorded payout (UX §12) — surface it as the
            // confirmation. Display only; the engine/server own the GL posting.
            emit payoutRecorded(QString::fromStdString(evt.payout_recorded().payout_id()),
                                QString::fromStdString(evt.payout_recorded().amount_str()),
                                QString::fromStdString(evt.payout_recorded().category()));
            break;
        case E::kAuthRequired:
            emit authRequired(QString::fromStdString(evt.auth_required().action()),
                              QString::fromStdString(evt.auth_required().reason()));
            break;
        case E::kReturnContextLoaded:
            // Full-snapshot reset of the returnable lines (same discipline as the cart),
            // then notify with the original receipt for the panel title.
            returnLines_->reset(evt.return_context_loaded());
            emit returnContextLoaded(
                QString::fromStdString(evt.return_context_loaded().original_receipt_no()));
            break;
        case E::kReturnCommitted:
            // The engine clears its return context on commit; mirror that here so the
            // returnable-lines model empties (the panel's "active" state ends).
            returnLines_->clear();
            emit returnCommitted(
                QString::fromStdString(evt.return_committed().credit_note_no()),
                evt.return_committed().provisional(),
                QString::fromStdString(evt.return_committed().total_str()),
                QString::fromStdString(evt.return_committed().tax_reversed_str()));
            break;
        case E::kRefundSettled:
            emit refundSettled(QString::fromStdString(evt.refund_settled().refund_no()),
                               evt.refund_settled().provisional(),
                               QString::fromStdString(evt.refund_settled().total_str()));
            break;
        case E::kHistoryResults:
            // Full-snapshot reset of the recent/search rows (same discipline as the cart),
            // then notify — the row payload IS the model, never a bare signal.
            history_->reset(evt.history_results());
            emit historyResults();
            break;
        case E::kBillDetail:
            // Recall (view) or reprint source — fill the detail model wholesale, then notify
            // with the receipt for the panel title.
            billDetail_->reset(evt.bill_detail());
            emit billDetailLoaded(QString::fromStdString(evt.bill_detail().receipt_no()));
            break;
        case E::kCartHeld:
            // The engine echoes the minted id after a hold (UX §10); surface it as the
            // hold confirmation. The cart itself clears via the following CartUpdated.
            emit cartHeld(QString::fromStdString(evt.cart_held().held_cart_id()),
                          QString::fromStdString(evt.cart_held().label()));
            break;
        case E::kHeldCartsList:
            // Full-snapshot reset of the active holds (same discipline as the cart/history),
            // then notify — the row payload IS the model, never a bare signal.
            heldCarts_->reset(evt.held_carts_list());
            emit heldCartsListed();
            break;
        default:
            break;  // events filled in as panels land
    }
}

// ── Command builders — thin: one oneof field set, then write ──────────────────
void PosEngineBridge::scanItem(const QString& barcode) {
    Command cmd;
    cmd.mutable_scan_item()->set_barcode(barcode.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::addLine(const QString& itemId, const QString& qty) {
    Command cmd;
    auto* m = cmd.mutable_add_line();
    m->set_item_id(itemId.toStdString());
    m->set_qty_str(qty.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::setQty(int lineNo, const QString& qty) {
    Command cmd;
    auto* m = cmd.mutable_set_qty();
    m->set_line_no(lineNo);
    m->set_qty_str(qty.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::setLineDiscount(int lineNo, const QString& discount) {
    Command cmd;
    auto* m = cmd.mutable_set_line_discount();
    m->set_line_no(lineNo);
    m->set_discount_str(discount.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::removeLine(int lineNo) {
    Command cmd;
    cmd.mutable_remove_line()->set_line_no(lineNo);
    writeCommand(std::move(cmd));
}

void PosEngineBridge::setOrderDiscount(const QString& discount) {
    Command cmd;
    cmd.mutable_set_order_discount()->set_discount_str(discount.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::addTender(const QString& method, const QString& amount,
                                const QString& reference) {
    Command cmd;
    auto* m = cmd.mutable_add_tender();
    m->set_method(method.toStdString());
    m->set_amount_str(amount.toStdString());
    m->set_reference(reference.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::removeTender(int tenderNo) {
    Command cmd;
    cmd.mutable_remove_tender()->set_tender_no(tenderNo);
    writeCommand(std::move(cmd));
}

void PosEngineBridge::settle() {
    Command cmd;
    cmd.mutable_settle();
    writeCommand(std::move(cmd));
}

void PosEngineBridge::voidCart() {
    Command cmd;
    cmd.mutable_void_cart();
    writeCommand(std::move(cmd));
}

void PosEngineBridge::addMiscCharge(const QString& description, const QString& amount) {
    Command cmd;
    auto* m = cmd.mutable_add_misc_charge();
    m->set_description(description.toStdString());
    m->set_amount_str(amount.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::recordPayout(const QString& amount, const QString& category,
                                   const QString& note) {
    Command cmd;
    auto* m = cmd.mutable_record_payout();
    m->set_amount_str(amount.toStdString());
    m->set_category(category.toStdString());
    m->set_note(note.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::startReturn(const QString& receiptNo, bool blind) {
    Command cmd;
    auto* m = cmd.mutable_start_return();
    m->set_original_receipt_no(receiptNo.toStdString());
    m->set_blind(blind);
    writeCommand(std::move(cmd));
}

void PosEngineBridge::setReturnLineQty(int originalLineNo, const QString& qty, bool restock) {
    Command cmd;
    auto* m = cmd.mutable_set_return_line_qty();
    m->set_original_line_no(originalLineNo);
    m->set_qty_str(qty.toStdString());
    m->set_restock(restock);
    writeCommand(std::move(cmd));
}

void PosEngineBridge::commitReturn(const QString& refundMethod) {
    Command cmd;
    cmd.mutable_commit_return()->set_refund_method(refundMethod.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::recallRecent(int limit) {
    Command cmd;
    cmd.mutable_recall_recent()->set_limit(limit);
    writeCommand(std::move(cmd));
}

void PosEngineBridge::recallByReceiptNo(const QString& receiptNo) {
    Command cmd;
    cmd.mutable_recall_by_receipt_no()->set_receipt_no(receiptNo.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::searchByCustomer(const QString& query) {
    Command cmd;
    cmd.mutable_search_by_customer()->set_query(query.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::reprintBill(const QString& receiptNo) {
    Command cmd;
    cmd.mutable_reprint_bill()->set_receipt_no(receiptNo.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::runEod() {
    Command cmd;
    cmd.mutable_run_eod();
    writeCommand(std::move(cmd));
}

void PosEngineBridge::holdCart(const QString& label) {
    Command cmd;
    cmd.mutable_hold_cart()->set_label(label.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::resumeCart(const QString& heldCartId) {
    Command cmd;
    cmd.mutable_resume_cart()->set_held_cart_id(heldCartId.toStdString());
    writeCommand(std::move(cmd));
}

void PosEngineBridge::listHeldCarts() {
    Command cmd;
    cmd.mutable_list_held_carts();
    writeCommand(std::move(cmd));
}

}  // namespace blissmont::bridge
