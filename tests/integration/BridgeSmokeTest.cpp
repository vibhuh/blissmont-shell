// tests/integration/BridgeSmokeTest.cpp — the spec §6 proof, end to end.
//
// Connects the real PosEngineBridge to a running TerminalEngine, scans an item, and waits for
// a CartUpdated to land in the model — one real Command round-tripping through real gRPC and
// rendering. Opt-in: set BLISSMONT_ENGINE_TARGET (e.g. "localhost:50080") to a live engine;
// otherwise the test SKIPs so a build box with no engine stays green. The engine can be the
// Go fakepos/devkit harness or the real headless engine from rachis-core.
#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <cstdlib>

#include "bridge/PosEngineBridge.hpp"
#include "core/Format.hpp"
#include "core/Money.hpp"
#include "models/CartLineModel.hpp"
#include "models/CartSummary.hpp"
#include "models/TenderListModel.hpp"
#include "models/ReturnLineModel.hpp"
#include "models/HistoryListModel.hpp"
#include "models/BillDetailModel.hpp"
#include "models/HeldCartModel.hpp"

using blissmont::bridge::PosEngineBridge;

namespace {
const char* engineTarget() { return std::getenv("BLISSMONT_ENGINE_TARGET"); }
}  // namespace

TEST(BridgeSmoke, ScanRoundTripsAndRenders) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));
    ASSERT_TRUE(bridge.connected());

    // The scanned line should land in the cart after the engine acks with CartUpdated.
    // Drain past the initial empty-cart snapshot the engine sends on connect — a bare wait
    // can return on that empty reset before the scan's CartUpdated arrives.
    QSignalSpy rowsSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && rowsSpy.wait(2000)) {
    }
    EXPECT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";
}

// The new tender wiring (contracts v1.2.0 shell build): a tender added over real gRPC
// lands in the TenderListModel, and removeTender takes it back out — both ride the
// engine's CartUpdated snapshot. Settle→OrderSettled is proven in the engine's own e2e
// (rachis-core) and needs a full shift lifecycle, so it is not duplicated here.
TEST(BridgeSmoke, TenderAddAndRemoveRoundTrip) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }
    using blissmont::models::TenderListModel;

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

    // Drain past the initial empty-cart snapshot the engine sends on connect: wait until
    // the SCANNED line is actually in the cart. A bare single wait can return on that empty
    // snapshot, leaving balanceDue at "0.00" — which the engine then rejects as a zero tender.
    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";

    // Tender the full balance → a tender row appears. Read balanceDue only now that the
    // scanned line is present, so it is the real (non-zero) balance the engine will accept.
    const QString balance = bridge.summary()->balanceDue();
    QSignalSpy tendersSpy(bridge.tenders(), &QAbstractItemModel::modelReset);
    bridge.addTender(QStringLiteral("cash"), balance, QString());
    while (bridge.tenders()->rowCount() == 0 && tendersSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.tenders()->rowCount(), 1)
        << "tender (amount " << balance.toStdString() << ") did not land";

    const int tenderNo =
        bridge.tenders()
            ->data(bridge.tenders()->index(0, 0), TenderListModel::TenderNoRole)
            .toInt();

    // Remove it → the tender row goes away.
    tendersSpy.clear();
    bridge.removeTender(tenderNo);
    ASSERT_TRUE(tendersSpy.wait(2000));
    EXPECT_EQ(bridge.tenders()->rowCount(), 0);

    bridge.disconnectFromEngine();
}

// Tier-0.1 trace (refinement brief): reproduce EXACTLY what SaleCompleteOverlay reads. The
// overlay's host (BillingScreen.onOrderSettled) captures summary.amountTendered/changeDue
// SYNCHRONOUSLY inside the orderSettled slot — before the engine's post-settle empty-cart
// CartUpdated is applied. A QSignalSpy.wait() would pump the loop past that empty snapshot and
// read 0.00 (a test artifact, not the real bug), so we connect a direct slot and capture the
// summary at emission time, which is what the QML handler actually sees. Over-tender so change
// is non-zero too. Classifies 0.1: if these are correct, the dialog path is display-correct.
TEST(BridgeSmoke, SettleOverlaySeesTenderSynchronously) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";

    const QString total = bridge.summary()->total();

    // Capture the summary the instant orderSettled fires — exactly as the overlay host does.
    QString capturedReceived, capturedChange, capturedTotalArg;
    QObject::connect(&bridge, &PosEngineBridge::orderSettled,
                     [&](const QString&, bool, const QString& totalArg) {
                         capturedTotalArg = totalArg;
                         capturedReceived = bridge.summary()->amountTendered();
                         capturedChange = bridge.summary()->changeDue();
                     });

    // Over-tender: received = total + 100, so change should be 100.00.
    const auto totalMinor = blissmont::core::Money::parse(total.toStdString());
    ASSERT_TRUE(totalMinor.ok());
    const auto received = totalMinor.value() + blissmont::core::Money(10000);
    const QString receivedStr = QString::fromStdString(received.toString());

    QSignalSpy settledSpy(&bridge, &PosEngineBridge::orderSettled);
    bridge.addTender(QStringLiteral("cash"), receivedStr, QString());
    bridge.settle();
    ASSERT_TRUE(settledSpy.wait(3000)) << "settle never produced OrderSettled (shift open?)";

    // OrderSettled.total_str is full-precision by design ("118.0000000000"); the overlay
    // formats it through Format.money, so compare by parsed value, not string.
    const auto capturedTotalMinor = blissmont::core::Money::parse(
        blissmont::core::numfmt::round(capturedTotalArg.toStdString(), 2));
    ASSERT_TRUE(capturedTotalMinor.ok());
    EXPECT_EQ(capturedTotalMinor.value().minorUnits(), totalMinor.value().minorUnits());
    EXPECT_EQ(capturedReceived.toStdString(), receivedStr.toStdString())
        << "overlay Received would show this";
    EXPECT_EQ(capturedChange.toStdString(), "100.00")
        << "overlay Change would show this";

    bridge.disconnectFromEngine();
}

// The returns shell build: drive a full return over real gRPC and observe the FULL
// provisional→canonical lifecycle. Requires the dev engine (cmd/blissmont-engine), which
// seeds an OPEN shift on boot (so the bridge's session hydrates it and can settle + commit a
// return) and drains its outbox in the background. Sell a line → settle (capture the receipt)
// → StartReturn → the returnable lines land in ReturnLineModel → SetReturnLineQty re-emits the
// context (snapshot reset) → CommitReturn → a provisional ReturnCommitted with a credit-note
// number → the background drain reconciles the refund and the canonical RefundSettled
// (provisional=false) lands. Both events of the two-event lifecycle are asserted.
TEST(BridgeSmoke, ReturnRoundTripProvisionalThenCanonical) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }
    using blissmont::models::ReturnLineModel;

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    QSignalSpy cfgSpy(&bridge, &PosEngineBridge::configUpdated);  // refund mode rides arg 7
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));
    // This test commits with no explicit refund method (original/cash auto-resolve). Under
    // "both" the engine requires a pick, so skip — that path is ReturnBothModePickThenCommit.
    if (cfgSpy.isEmpty()) cfgSpy.wait(2000);
    if (!cfgSpy.isEmpty() && cfgSpy.at(0).at(7).toString() == QStringLiteral("both")) {
        GTEST_SKIP() << "engine in refund_tender_mode=both; covered by the both-mode test";
    }

    // Sell one line (drain past the initial empty-cart snapshot first).
    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";

    // Tender the full balance and settle; capture the provisional receipt number.
    QSignalSpy settledSpy(&bridge, &PosEngineBridge::orderSettled);
    bridge.addTender(QStringLiteral("cash"), bridge.summary()->balanceDue(), QString());
    bridge.settle();
    ASSERT_TRUE(settledSpy.wait(3000)) << "settle never produced OrderSettled (shift open?)";
    const QString receiptNo = settledSpy.at(0).at(0).toString();
    ASSERT_FALSE(receiptNo.isEmpty());

    // StartReturn → the original bill's returnable lines land in the model.
    QSignalSpy ctxSpy(&bridge, &PosEngineBridge::returnContextLoaded);
    QSignalSpy returnLinesSpy(bridge.returnLines(), &QAbstractItemModel::modelReset);
    bridge.startReturn(receiptNo, /*blind=*/false);
    ASSERT_TRUE(ctxSpy.wait(3000)) << "StartReturn never produced ReturnContextLoaded";
    EXPECT_EQ(ctxSpy.at(0).at(0).toString(), receiptNo);
    ASSERT_GE(bridge.returnLines()->rowCount(), 1) << "no returnable lines loaded";

    const int lineNo =
        bridge.returnLines()
            ->data(bridge.returnLines()->index(0, 0), ReturnLineModel::OriginalLineNoRole)
            .toInt();

    // SetReturnLineQty re-emits ReturnContextLoaded — the model resets again.
    returnLinesSpy.clear();
    bridge.setReturnLineQty(lineNo, QStringLiteral("1"), /*restock=*/true);
    ASSERT_TRUE(returnLinesSpy.wait(2000)) << "SetReturnLineQty did not re-emit the context";
    ASSERT_GE(bridge.returnLines()->rowCount(), 1);

    // CommitReturn → a provisional credit note, and the model clears. Arm the canonical
    // spy first — the background drain can reconcile and emit RefundSettled quickly.
    QSignalSpy committedSpy(&bridge, &PosEngineBridge::returnCommitted);
    QSignalSpy refundSettledSpy(&bridge, &PosEngineBridge::refundSettled);
    bridge.commitReturn();
    ASSERT_TRUE(committedSpy.wait(3000)) << "CommitReturn never produced ReturnCommitted";
    const QString creditNoteNo = committedSpy.at(0).at(0).toString();
    const bool provisional = committedSpy.at(0).at(1).toBool();
    EXPECT_FALSE(creditNoteNo.isEmpty()) << "credit note number missing";
    EXPECT_TRUE(provisional) << "the commit event is always the provisional half";

    // Canonical half: the background drain syncs the refund and emits RefundSettled with
    // provisional=false and a server refund number. Drain past any provisional RefundSettled.
    bool sawCanonical = false;
    QString canonicalRefundNo;
    while (refundSettledSpy.wait(4000)) {
        const auto row = refundSettledSpy.takeFirst();
        if (!row.at(1).toBool()) {  // provisional=false → canonical
            sawCanonical = true;
            canonicalRefundNo = row.at(0).toString();
            break;
        }
    }
    EXPECT_TRUE(sawCanonical) << "canonical RefundSettled never arrived (is the engine draining?)";
    EXPECT_FALSE(canonicalRefundNo.isEmpty()) << "canonical refund number missing";

    bridge.disconnectFromEngine();
}

// The history shell build over real gRPC: sell + settle a bill, then exercise the four
// local-first reads. RecallRecent fills the HistoryListModel; the settled receipt is among the
// recent rows; RecallByReceiptNo fills the BillDetailModel with that bill's lines + payments;
// ReprintBill re-emits BillDetail (the DUPLICATE banner is on the engine's printed bytes, not
// the event). Requires the dev engine (seeds an OPEN shift on boot) so the bill can settle.
TEST(BridgeSmoke, HistoryRecallReprintRoundTrip) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }
    using blissmont::models::HistoryListModel;

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

    // Sell one line, tender the balance, and settle; capture the receipt.
    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";
    QSignalSpy settledSpy(&bridge, &PosEngineBridge::orderSettled);
    bridge.addTender(QStringLiteral("cash"), bridge.summary()->balanceDue(), QString());
    bridge.settle();
    ASSERT_TRUE(settledSpy.wait(3000)) << "settle never produced OrderSettled (shift open?)";
    const QString receiptNo = settledSpy.at(0).at(0).toString();
    ASSERT_FALSE(receiptNo.isEmpty());

    // RecallRecent → the recent bills land in the history model, including our settled bill.
    QSignalSpy historySpy(&bridge, &PosEngineBridge::historyResults);
    bridge.recallRecent(20);
    ASSERT_TRUE(historySpy.wait(3000)) << "RecallRecent never produced HistoryResults";
    ASSERT_GE(bridge.history()->rowCount(), 1) << "no recent bills returned";
    bool found = false;
    for (int i = 0; i < bridge.history()->rowCount(); ++i) {
        const QString r = bridge.history()
                              ->data(bridge.history()->index(i, 0),
                                     HistoryListModel::ReceiptNoRole)
                              .toString();
        if (r == receiptNo) { found = true; break; }
    }
    EXPECT_TRUE(found) << "the settled receipt was not among the recent bills";

    // RecallByReceiptNo → the bill's detail (lines + payments) lands in the detail model.
    QSignalSpy detailSpy(&bridge, &PosEngineBridge::billDetailLoaded);
    bridge.recallByReceiptNo(receiptNo);
    ASSERT_TRUE(detailSpy.wait(3000)) << "RecallByReceiptNo never produced BillDetail";
    EXPECT_EQ(detailSpy.at(0).at(0).toString(), receiptNo);
    EXPECT_TRUE(bridge.billDetail()->active());
    EXPECT_EQ(bridge.billDetail()->receiptNo(), receiptNo);
    EXPECT_GE(bridge.billDetail()->lines()->rowCount(), 1) << "recalled bill has no lines";
    EXPECT_GE(bridge.billDetail()->payments()->rowCount(), 1) << "recalled bill has no payments";

    // ReprintBill → the engine re-emits BillDetail (and prints a DUPLICATE receipt).
    detailSpy.clear();
    bridge.reprintBill(receiptNo);
    ASSERT_TRUE(detailSpy.wait(3000)) << "ReprintBill never produced BillDetail";
    EXPECT_EQ(detailSpy.at(0).at(0).toString(), receiptNo);

    bridge.disconnectFromEngine();
}

// The suspend/resume shell build (contracts v1.4.0) over real gRPC against a live engine.
// Build a cart → holdCart echoes the minted id (cartHeld) and the cart clears → listHeldCarts
// fills the HeldCartModel with the parked draft → resumeCart restores the cart and the engine
// re-emits CartUpdated with status="held". Exercises the same full-snapshot reset discipline on
// the bridge's reader thread where tender/returns had race bugs — against the draining engine.
TEST(BridgeSmoke, SuspendResumeRoundTrip) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }
    using blissmont::models::HeldCartModel;

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

    // Build a cart (drain past the initial empty-cart snapshot the engine sends on connect).
    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "scanned line never arrived in the cart";

    // Suspend it → the engine echoes the minted id (cartHeld) AND clears the live cart.
    QSignalSpy heldSpy(&bridge, &PosEngineBridge::cartHeld);
    cartSpy.clear();
    bridge.holdCart(QStringLiteral("counter 2"));
    ASSERT_TRUE(heldSpy.wait(3000)) << "holdCart never produced cartHeld";
    const QString heldId = heldSpy.at(0).at(0).toString();
    EXPECT_FALSE(heldId.isEmpty()) << "cartHeld carried no id — the minted id was lost";
    EXPECT_EQ(heldSpy.at(0).at(1).toString(), QStringLiteral("counter 2"));
    // The cart clears for the next customer (the post-hold empty CartUpdated).
    while (bridge.cart()->rowCount() > 0 && cartSpy.wait(2000)) {
    }
    EXPECT_EQ(bridge.cart()->rowCount(), 0) << "the cart did not clear after the hold";

    // Discover the hold → it lands in the HeldCartModel (full snapshot).
    QSignalSpy listSpy(&bridge, &PosEngineBridge::heldCartsListed);
    bridge.listHeldCarts();
    ASSERT_TRUE(listSpy.wait(3000)) << "listHeldCarts never produced HeldCartsList";
    ASSERT_GE(bridge.heldCarts()->rowCount(), 1) << "no held carts returned";
    bool found = false;
    for (int i = 0; i < bridge.heldCarts()->rowCount(); ++i) {
        const QString id = bridge.heldCarts()
                               ->data(bridge.heldCarts()->index(i, 0),
                                      HeldCartModel::HeldCartIdRole)
                               .toString();
        if (id == heldId) { found = true; break; }
    }
    EXPECT_TRUE(found) << "the held cart was not among the listed holds";

    // Resume it → the engine restores the cart and re-emits CartUpdated with status="held".
    cartSpy.clear();
    bridge.resumeCart(heldId);
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1) << "resume did not restore the cart";
    EXPECT_EQ(bridge.summary()->status(), QStringLiteral("held"))
        << "a resumed draft must report status=held";

    bridge.disconnectFromEngine();
}

// The Tasks-launcher Cash In path over real gRPC: recordCashMovement("cash_in", …) round-trips
// the existing record_cash_movement command (proto tag 17) and the engine echoes
// CashMovementRecorded with the type + the amount it accepted. Proves the new bridge wiring
// (no contract change) reaches the engine's existing handler. Requires the dev engine.
TEST(BridgeSmoke, CashInRecordsAndEchoes) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

    QSignalSpy cashSpy(&bridge, &PosEngineBridge::cashMovementRecorded);
    bridge.recordCashMovement(QStringLiteral("cash_in"), QStringLiteral("500"),
                              QStringLiteral("opening float top-up"));
    ASSERT_TRUE(cashSpy.wait(3000)) << "recordCashMovement never produced CashMovementRecorded";
    EXPECT_EQ(cashSpy.at(0).at(1).toString(), QStringLiteral("cash_in"));
    EXPECT_FALSE(cashSpy.at(0).at(2).toString().isEmpty()) << "echoed amount missing";

    bridge.disconnectFromEngine();
}

// Phase B over real gRPC: under refund_tender_mode="both" the cashier must pick the refund
// tender. Committing with no choice is rejected (REFUND_METHOD_REQUIRED); committing with an
// explicit "cash" choice issues the credit note. Requires the dev engine started with
// BLISSMONT_REFUND_TENDER_MODE=both; otherwise this skips (the engine drives the mode).
TEST(BridgeSmoke, ReturnBothModePickThenCommit) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }
    using blissmont::models::ReturnLineModel;

    PosEngineBridge bridge;
    QSignalSpy connSpy(&bridge, &PosEngineBridge::connectionChanged);
    QSignalSpy cfgSpy(&bridge, &PosEngineBridge::configUpdated);
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));
    if (cfgSpy.isEmpty()) cfgSpy.wait(2000);
    if (cfgSpy.isEmpty() || cfgSpy.at(0).at(7).toString() != QStringLiteral("both")) {
        GTEST_SKIP() << "engine not in refund_tender_mode=both (set BLISSMONT_REFUND_TENDER_MODE=both)";
    }

    // Sell + settle a bill to return against.
    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    while (bridge.cart()->rowCount() == 0 && cartSpy.wait(2000)) {
    }
    ASSERT_GE(bridge.cart()->rowCount(), 1);
    QSignalSpy settledSpy(&bridge, &PosEngineBridge::orderSettled);
    bridge.addTender(QStringLiteral("cash"), bridge.summary()->balanceDue(), QString());
    bridge.settle();
    ASSERT_TRUE(settledSpy.wait(3000));
    const QString receiptNo = settledSpy.at(0).at(0).toString();
    ASSERT_FALSE(receiptNo.isEmpty());

    // Load the return + select the line.
    QSignalSpy ctxSpy(&bridge, &PosEngineBridge::returnContextLoaded);
    QSignalSpy returnLinesSpy(bridge.returnLines(), &QAbstractItemModel::modelReset);
    bridge.startReturn(receiptNo, /*blind=*/false);
    ASSERT_TRUE(ctxSpy.wait(3000));
    ASSERT_GE(bridge.returnLines()->rowCount(), 1);
    const int lineNo =
        bridge.returnLines()
            ->data(bridge.returnLines()->index(0, 0), ReturnLineModel::OriginalLineNoRole)
            .toInt();
    returnLinesSpy.clear();
    bridge.setReturnLineQty(lineNo, QStringLiteral("1"), /*restock=*/true);
    ASSERT_TRUE(returnLinesSpy.wait(2000));

    // (1) Commit with NO chosen method → the engine rejects it; no credit note.
    QSignalSpy rejectedSpy(&bridge, &PosEngineBridge::commandRejected);
    QSignalSpy committedSpy(&bridge, &PosEngineBridge::returnCommitted);
    bridge.commitReturn(QString());
    ASSERT_TRUE(rejectedSpy.wait(2000)) << "empty refund method should be rejected under 'both'";
    EXPECT_EQ(rejectedSpy.at(0).at(0).toString(), QStringLiteral("REFUND_METHOD_REQUIRED"));
    EXPECT_TRUE(committedSpy.isEmpty()) << "no credit note may be issued on the rejected commit";

    // (2) Commit with an explicit "cash" choice → the credit note is issued.
    bridge.commitReturn(QStringLiteral("cash"));
    ASSERT_TRUE(committedSpy.wait(3000)) << "picked commit never produced ReturnCommitted";
    EXPECT_FALSE(committedSpy.at(0).at(0).toString().isEmpty()) << "credit note number missing";

    bridge.disconnectFromEngine();
}
