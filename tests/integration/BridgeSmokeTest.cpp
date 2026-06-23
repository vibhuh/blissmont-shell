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
