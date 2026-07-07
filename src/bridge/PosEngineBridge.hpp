// bridge/PosEngineBridge.hpp — the ONLY component that touches gRPC (spec §4).
//
// One QObject, exposed to QML as a singleton. Owns the localhost gRPC channel to the
// engine and the bidirectional `Session` stream. UI -> engine is a set of thin Q_INVOKABLE
// methods that each build one Command oneof and write it. Engine -> UI is: cart lines via
// CartLineModel (full reset per snapshot), totals via CartSummary Q_PROPERTYs, and discrete
// events via Qt signals. The engine is the single source of truth; the bridge never mutates
// cart state itself — it renders snapshots.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QVariantList>

#include <atomic>
#include <deque>
#include <memory>

#include <grpcpp/grpcpp.h>
#include "terminal/v1/terminal.grpc.pb.h"

#include "models/CartLineModel.hpp"
#include "models/CartSummary.hpp"
#include "models/TenderListModel.hpp"
#include "models/ReturnLineModel.hpp"
#include "models/HistoryListModel.hpp"
#include "models/BillDetailModel.hpp"
#include "models/HeldCartModel.hpp"
#include "models/OperatorModel.hpp"

namespace blissmont::bridge {

class PosEngineBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(blissmont::models::CartLineModel* cart READ cart CONSTANT)
    Q_PROPERTY(blissmont::models::CartSummary* summary READ summary CONSTANT)
    Q_PROPERTY(blissmont::models::TenderListModel* tenders READ tenders CONSTANT)
    Q_PROPERTY(blissmont::models::ReturnLineModel* returnLines READ returnLines CONSTANT)
    Q_PROPERTY(blissmont::models::HistoryListModel* history READ history CONSTANT)
    Q_PROPERTY(blissmont::models::BillDetailModel* billDetail READ billDetail CONSTANT)
    Q_PROPERTY(blissmont::models::HeldCartModel* heldCarts READ heldCarts CONSTANT)
    Q_PROPERTY(blissmont::models::OperatorModel* operators READ operators CONSTANT)
    // The device's product catalogue for the search panel (client-side ranked lookup via
    // LookupController). A QVariantList of QVariantMap rows {id,name,sku,barcode,price,hsn,gst,
    // category}; refreshed on each ProductList reply. Empty until listProducts() is answered.
    Q_PROPERTY(QVariantList products READ products NOTIFY productsListed)
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)

public:
    explicit PosEngineBridge(QObject* parent = nullptr);
    ~PosEngineBridge() override;

    [[nodiscard]] blissmont::models::CartLineModel* cart() const { return cart_; }
    [[nodiscard]] blissmont::models::CartSummary* summary() const { return summary_; }
    [[nodiscard]] blissmont::models::TenderListModel* tenders() const { return tenders_; }
    [[nodiscard]] blissmont::models::ReturnLineModel* returnLines() const { return returnLines_; }
    [[nodiscard]] blissmont::models::HistoryListModel* history() const { return history_; }
    [[nodiscard]] blissmont::models::BillDetailModel* billDetail() const { return billDetail_; }
    [[nodiscard]] blissmont::models::HeldCartModel* heldCarts() const { return heldCarts_; }
    [[nodiscard]] blissmont::models::OperatorModel* operators() const { return operators_; }
    [[nodiscard]] QVariantList products() const { return products_; }
    [[nodiscard]] bool connected() const { return connected_.load(std::memory_order_relaxed); }

    // ── UI -> engine (one per Command oneof; thin: build + write) ─────────────
    Q_INVOKABLE void connectToEngine(const QString& target = QStringLiteral("localhost:50080"));
    Q_INVOKABLE void disconnectFromEngine();

    Q_INVOKABLE void scanItem(const QString& barcode);
    Q_INVOKABLE void addLine(const QString& itemId, const QString& qty);
    Q_INVOKABLE void setQty(int lineNo, const QString& qty);
    Q_INVOKABLE void setLineDiscount(int lineNo, const QString& discount);
    // Per-line unit-price override (proto set_price_override, tag 5) — exposed for the
    // expanded-row line action. The engine still gates it on terminal perms.
    Q_INVOKABLE void setPriceOverride(int lineNo, const QString& price);
    Q_INVOKABLE void removeLine(int lineNo);
    Q_INVOKABLE void setOrderDiscount(const QString& discount);
    // Select a customer for the bill (proto select_customer, tag 8); empty id = walk-in.
    Q_INVOKABLE void selectCustomer(const QString& posCustomerId);
    Q_INVOKABLE void addTender(const QString& method, const QString& amount, const QString& reference);
    Q_INVOKABLE void removeTender(int tenderNo);
    Q_INVOKABLE void settle();
    Q_INVOKABLE void voidCart();
    Q_INVOKABLE void addMiscCharge(const QString& description, const QString& amount);
    // Cash drawer movement (UX §12) — type is "cash_in" | "cash_out". Surfaces the
    // existing record_cash_movement command (proto tag 17, already handled engine-side)
    // to the Tasks "Cash In" launcher. No contract change: the proto + engine path exist;
    // this is the missing view-boundary wiring. The engine echoes CashMovementRecorded.
    Q_INVOKABLE void recordCashMovement(const QString& type, const QString& amount,
                                        const QString& reason);
    Q_INVOKABLE void recordPayout(const QString& amount, const QString& category, const QString& note);
    Q_INVOKABLE void startReturn(const QString& receiptNo, bool blind);
    Q_INVOKABLE void setReturnLineQty(int originalLineNo, const QString& qty, bool restock);
    // refundMethod is the cashier's choice ("original" | "cash") under
    // refund_tender_mode="both"; empty for original/cash modes (engine resolves).
    Q_INVOKABLE void commitReturn(const QString& refundMethod = QString());
    // History (UX §10) — all local-first reads. recallRecent / searchByCustomer fill the
    // history model; recallByReceiptNo / reprintBill fill the billDetail model (reprint also
    // marks DUPLICATE engine-side on the printed receipt).
    Q_INVOKABLE void recallRecent(int limit);
    Q_INVOKABLE void recallByReceiptNo(const QString& receiptNo);
    Q_INVOKABLE void searchByCustomer(const QString& query);
    Q_INVOKABLE void reprintBill(const QString& receiptNo);
    Q_INVOKABLE void runEod();
    // Begin-Day (UX §12) — open the day's shift with an opening float. The engine emits
    // ShiftStateChanged(open) on success, or CommandRejected(SHIFT_ALREADY_OPEN) if one is
    // already open. cashierUserId identifies the opening cashier (no shell login yet — a
    // device-default is passed for now).
    // shiftMasterId classifies a SCHEDULED-mode session (empty in SINGLE/MULTIPLE). authReason +
    // authorizedBy carry a supervisor attestation on a RE-ISSUE after AuthRequired(open_session_policy):
    // the shell verifies the supervisor device-side (attestation-only for now — no device credential
    // store yet), and the engine trusts the same-device SupervisorAuth and skips the policy gate. Both
    // empty on the first (ungated) attempt.
    // operatorPin (Slice B): the cashier's Begin-Day PIN. The engine bcrypt-verifies it against
    // the locally-synced operator cache (offline) and rejects a wrong/unknown/disabled operator
    // with commandRejected BEFORE opening — so cashierUserId (a real users.id from the picker)
    // only reaches the backend's opened_by after a real operator authenticated. Empty when the
    // device has no synced operators yet (the engine then accepts the id as-is).
    Q_INVOKABLE void openShift(const QString& cashierUserId, const QString& openingCashStr,
                               const QString& shiftMasterId = QString(),
                               const QString& authReason = QString(),
                               const QString& authorizedBy = QString(),
                               const QString& operatorPin = QString());
    // Shift close (UX §12, blind denomination count). denominations is a list of {unit, count}
    // maps (rupee note/coin value → count keyed). The engine sums them (Σ unit×count) as the
    // counted cash, reconciles it against EXPECTED cash (opening + cash sales + cash-in −
    // cash-out − payouts − refunds — the same figure the Z-report prints), computes variance =
    // counted − expected, and replies with ShiftClosed (figures revealed only AFTER commit), or
    // AuthRequired(close_shift_variance) when the variance exceeds tolerance and the config
    // demands sign-off. closingCashStr carries the same total for a single-total count mode.
    // authReason + authorizedBy carry a supervisor attestation on a RE-ISSUE after
    // AuthRequired(close_shift_variance) — same mechanism as openShift's SupervisorAuth. Both empty on
    // the first (ungated) close; the engine holds an over-tolerance variance until they're supplied.
    Q_INVOKABLE void closeShift(const QVariantList& denominations, const QString& closingCashStr,
                                const QString& authReason = QString(),
                                const QString& authorizedBy = QString());
    // Suspend/resume (UX §10) — drafts are terminal-local. holdCart parks the current cart
    // (the engine echoes the minted id via cartHeld); resumeCart restores one by id (the
    // engine re-emits CartUpdated with status="held"); listHeldCarts fills the held-cart model.
    Q_INVOKABLE void holdCart(const QString& label);
    Q_INVOKABLE void resumeCart(const QString& heldCartId);
    Q_INVOKABLE void listHeldCarts();
    // Operator login (Slice B) — fills the operators model with the device's enabled cashiers
    // (engine replies OperatorsList → operatorsListed). Reads the local operator cache; works
    // fully offline. The shell requests this when the Begin-Day screen opens.
    Q_INVOKABLE void listOperators();
    // Catalog search — fills the products list with the device's catalogue (engine replies
    // ProductList → productsListed). Reads the local product cache; works fully offline. The
    // shell requests this when the product-search panel opens.
    Q_INVOKABLE void listProducts();

signals:
    void connectionChanged();
    // received/change carry the settle-time tender (contracts v1.6.0) so the post-settle
    // confirmation binds to immutable values, not the live cart summary the engine resets.
    void orderSettled(const QString& receiptNo, bool provisional, const QString& total,
                      const QString& received, const QString& change);
    void itemNotFound(const QString& barcode);
    void commandRejected(const QString& code, const QString& message);
    void shiftStateChanged(const QString& shiftId, const QString& status);
    // Blind-count reveal after a successful CloseShift: the opening float, the counted cash, the
    // variance (counted − expected; negative = short, positive = over), and — additive in
    // terminal v1.9.0 — the full drawer reconciliation the variance is measured against:
    // expected cash and the components (cash sales, cash-in/out, payouts, refunds) that make it up.
    void shiftClosed(const QString& openingFloat, const QString& countedCash, const QString& variance,
                     const QString& expectedCash, const QString& cashSales, const QString& cashIn,
                     const QString& cashOut, const QString& payouts, const QString& refunds);
    void syncStatusChanged(bool online, int pending);
    // Device config relayed by the engine over the Session stream (contracts
    // v1.1.0; payment methods added in v1.2.0). Emitted on connect, on reconnect,
    // and on every config change, with the device-domain fields the UI gates on.
    // Carries plain Qt types so services stay free of any gRPC/proto dependency
    // (the bridge owns that translation). paymentMethods is a QVariantList of
    // QVariantMap rows {method, displayName, hotkey, sortOrder, enabled,
    // referenceMode} — the device-surface fields only; tender secrets never cross.
    // The returns policy axes (allowBlindReturn … allowPartialReturn) ride the same
    // ConfigUpdated relay as the tender block — device-domain scalars the panel gates
    // copy/affordances on (the engine still enforces every one). Added in the returns
    // shell build; ConfigService projects them to QML.
    void configUpdated(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                       const QString& tenderCompleteMode, const QString& currencySymbol,
                       const QVariantList& paymentMethods,
                       bool allowBlindReturn, const QString& refundTenderMode,
                       const QString& returnRequiresAuth, bool restockDefault,
                       bool allowPartialReturn, const QString& heldCartExpiry,
                       const QStringList& payoutCategories,
                       // Store + register identity for the top bar (contracts v1.6.0:
                       // store_name already on the wire; register_name device-local).
                       const QString& storeName, const QString& registerName,
                       // Shift management (contracts v1.10.0): the mode drives which Begin-Register
                       // screen the shell shows; the four require_auth_* bools are the uniform
                       // supervisor-auth policy the engine gates SCHEDULED opens on; shiftMasters is
                       // the company's shift definitions (QVariantList of QVariantMap
                       // {id, code, name, startTime, endTime, sequence, isActive}), non-empty only in
                       // SCHEDULED mode. ConfigService projects them to QML.
                       const QString& shiftManagementMode,
                       bool requireAuthBeforeStart, bool requireAuthAfterEnd,
                       bool requireAuthDifferentShift, bool requireAuthReopenCompleted,
                       const QVariantList& shiftMasters);
    void authRequired(const QString& action, const QString& reason);
    // A payout was recorded (UX §12): the engine echoes the provisional/local payout id,
    // amount and category after RecordPayout. The shell surfaces this as the payout
    // confirmation — display only; the engine + server own the GL posting. The amount is
    // exactly what the engine accepted (never re-keyed here).
    void payoutRecorded(const QString& payoutId, const QString& amount, const QString& category);
    // A cash drawer movement was recorded (UX §12): the engine echoes the provisional/local
    // movement id, type ("cash_in"|"cash_out") and amount after RecordCashMovement. Display
    // only — the engine + server own the GL posting. Mirrors payoutRecorded.
    void cashMovementRecorded(const QString& movementId, const QString& type,
                              const QString& amount);
    // EOD day-close outcome (UX §12). eodResult carries the batch id (provisional until sync);
    // eodBlocked carries the still-open shift ids that prevented the close. Surfaced by the
    // Tasks "Z Report" launcher as a confirmation / blocking notice (display only — the engine
    // owns the close). The proto + engine paths (RunEod / EodResult / EodBlocked) already exist.
    void eodResult(const QString& batchId, bool provisional);
    void eodBlocked(const QStringList& openShiftIds);
    // The original bill's returnable lines have landed in returnLines (full snapshot).
    // Carries the original receipt for the panel title; the line payload is the model.
    void returnContextLoaded(const QString& originalReceiptNo);
    // Provisional credit note issued on CommitReturn (canonical refund follows on sync
    // via refundSettled). credit_note_no / tax_reversed are provisional until then.
    void returnCommitted(const QString& creditNoteNo, bool provisional,
                         const QString& total, const QString& taxReversed);
    // Canonical reconcile of a refund/credit note on sync (slice-2 event). Shares no
    // key with returnCommitted — see the two-event lifecycle in the build brief; the
    // shell uses it for display status only, never to gate print/navigation.
    void refundSettled(const QString& refundNo, bool provisional, const QString& total);
    // A fresh recent/search result set has landed in the history model (full snapshot). The
    // row payload is the model; this is the "results changed" notify.
    void historyResults();
    // A recalled / reprinted bill's detail has landed in the billDetail model (full snapshot).
    // Carries the receipt for the panel title; fires for BOTH recallByReceiptNo (view) and
    // reprintBill (the engine re-emits BillDetail after printing) — the event carries no
    // duplicate flag, so the two are distinguished by the caller's intent, not the payload.
    // Named *Loaded to avoid clashing with the billDetail() property getter.
    void billDetailLoaded(const QString& receiptNo);
    // A cart was suspended (UX §10): the engine echoes the minted held-cart id (it was
    // previously discarded). The shell surfaces this as the hold confirmation.
    void cartHeld(const QString& heldCartId, const QString& label);
    // A fresh active-holds set has landed in the heldCarts model (full snapshot). The row
    // payload IS the model — this is the "holds changed" notify (mirrors historyResults).
    void heldCartsListed();
    // A fresh operator set has landed in the operators model (full snapshot). The row payload
    // IS the model — this is the "operators changed" notify for the Begin-Day picker.
    void operatorsListed();
    // A fresh product catalogue has landed in the products list (full snapshot). The search
    // panel re-feeds its LookupController from products on this notify.
    void productsListed();

private:
    using Command = blissmont::terminal::v1::Command;
    using Event = blissmont::terminal::v1::Event;
    using Stream = grpc::ClientReaderWriter<Command, Event>;

    void writeCommand(Command cmd);          // thread-safe writer (1 writer)
    void readLoop();                         // worker thread (1 reader)
    void applyEvent(const Event& evt);       // UI thread (marshaled via QueuedConnection)
    void setConnected(bool value);

    blissmont::models::CartLineModel* cart_;
    blissmont::models::CartSummary* summary_;
    blissmont::models::TenderListModel* tenders_;
    blissmont::models::ReturnLineModel* returnLines_;
    blissmont::models::HistoryListModel* history_;
    blissmont::models::BillDetailModel* billDetail_;
    blissmont::models::HeldCartModel* heldCarts_;
    blissmont::models::OperatorModel* operators_;
    QVariantList products_;

    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<blissmont::terminal::v1::TerminalEngine::Stub> stub_;
    std::unique_ptr<grpc::ClientContext> ctx_;
    std::unique_ptr<Stream> stream_;

    std::unique_ptr<QThread> readerThread_;
    QMutex writeMutex_;
    // Commands issued before the Session stream exists (e.g. a panel requesting the
    // catalog in Component.onCompleted, which runs before Main.qml's connectToEngine)
    // are queued here and flushed on connect instead of being silently dropped.
    // Bounded — the oldest is dropped past the cap so a stream that never comes up
    // can't grow this without limit. Guarded by writeMutex_.
    static constexpr int kMaxPendingWrites = 64;
    std::deque<Command> pendingWrites_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> stopping_{false};
};

}  // namespace blissmont::bridge
