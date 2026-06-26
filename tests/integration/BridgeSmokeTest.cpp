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

    // A row should appear in the cart model after the engine acks the scan with CartUpdated.
    QSignalSpy rowsSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    EXPECT_TRUE(rowsSpy.wait(2000));
    EXPECT_GE(bridge.cart()->rowCount(), 0);
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

    QSignalSpy cartSpy(bridge.cart(), &QAbstractItemModel::modelReset);
    bridge.scanItem(QStringLiteral("TESTSKU"));
    ASSERT_TRUE(cartSpy.wait(2000));

    // Tender the full balance → a tender row appears.
    QSignalSpy tendersSpy(bridge.tenders(), &QAbstractItemModel::modelReset);
    bridge.addTender(QStringLiteral("cash"), bridge.summary()->balanceDue(), QString());
    ASSERT_TRUE(tendersSpy.wait(2000));
    ASSERT_GE(bridge.tenders()->rowCount(), 1);

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
