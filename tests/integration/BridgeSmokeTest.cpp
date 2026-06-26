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
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(connSpy.wait(2000));

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
