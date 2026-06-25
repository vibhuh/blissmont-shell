// bridge/PosEngineBridge.cpp — see PosEngineBridge.hpp.
#include "bridge/PosEngineBridge.hpp"

#include <QMetaObject>

#include <utility>

namespace blissmont::bridge {

PosEngineBridge::PosEngineBridge(QObject* parent)
    : QObject(parent),
      cart_(new blissmont::models::CartLineModel(this)),
      summary_(new blissmont::models::CartSummary(this)) {}

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
            emit configUpdated(cfg.allow_returns(), cfg.payout_enabled(),
                               cfg.allow_discounts(),
                               QString::fromStdString(cfg.tender_complete_mode()),
                               QString::fromStdString(cfg.currency_symbol()));
            break;
        }
        case E::kAuthRequired:
            emit authRequired(QString::fromStdString(evt.auth_required().action()),
                              QString::fromStdString(evt.auth_required().reason()));
            break;
        case E::kReturnContextLoaded:
            emit returnContextLoaded();
            break;
        case E::kHistoryResults:
            emit historyResults();
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

void PosEngineBridge::runEod() {
    Command cmd;
    cmd.mutable_run_eod();
    writeCommand(std::move(cmd));
}

}  // namespace blissmont::bridge
