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
#include "models/CartLineModel.hpp"
#include "models/TenderListModel.hpp"
#include "models/ReturnLineModel.hpp"
#include "models/HistoryListModel.hpp"
#include "models/BillDetailModel.hpp"

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

// The returns shell build: drive a full return over real gRPC against a live engine.
// Requires the dev engine (cmd/blissmont-engine), which seeds an OPEN shift on boot, so
// the bridge's session hydrates it and can settle + commit a return. Sell a line → settle
// (capture the receipt) → StartReturn → the returnable lines land in ReturnLineModel →
// SetReturnLineQty re-emits the context → CommitReturn → a provisional ReturnCommitted with
// a credit-note number. The canonical RefundSettled is produced by the syncer's outbox drain
// (the dev engine does not drain), so the provisional commit is the round-trip asserted here.
TEST(BridgeSmoke, ReturnRoundTripIssuesProvisionalCreditNote) {
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

    // CommitReturn → a provisional credit note, and the model clears.
    QSignalSpy committedSpy(&bridge, &PosEngineBridge::returnCommitted);
    bridge.commitReturn();
    ASSERT_TRUE(committedSpy.wait(3000)) << "CommitReturn never produced ReturnCommitted";
    const QString creditNoteNo = committedSpy.at(0).at(0).toString();
    const bool provisional = committedSpy.at(0).at(1).toBool();
    EXPECT_FALSE(creditNoteNo.isEmpty()) << "credit note number missing";
    EXPECT_TRUE(provisional) << "offline commit should be provisional until sync";

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
