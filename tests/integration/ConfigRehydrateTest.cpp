// tests/integration/ConfigRehydrateTest.cpp — the v1.1.0 reconnect proof, end to end.
//
// Connects the real PosEngineBridge to a running TerminalEngine and asserts that
// ConfigUpdated arrives on connect, then DROPS the session and re-establishes it and
// asserts ConfigUpdated arrives AGAIN — config auto-rehydrates over the same Session
// path as Cart/Shift/Sync. The reconnect path is where hydration bugs live, so it is
// asserted explicitly, not just on first connect.
//
// Opt-in: set BLISSMONT_ENGINE_TARGET (e.g. "localhost:50080") to a live engine that
// has resolved its config (the rachis-core headless engine after a RefreshConfig, or
// the fakepos/devkit harness). Skips otherwise so a build box with no engine stays green.
#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <cstdlib>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConfigService.hpp"

using blissmont::bridge::PosEngineBridge;
using blissmont::services::ConfigService;

namespace {
const char* engineTarget() { return std::getenv("BLISSMONT_ENGINE_TARGET"); }
}  // namespace

TEST(ConfigRehydrate, ConfigRepopulatesAfterReconnect) {
    const char* target = engineTarget();
    if (!target) {
        GTEST_SKIP() << "set BLISSMONT_ENGINE_TARGET to a running engine to run this";
    }

    PosEngineBridge bridge;
    ConfigService config;
    // Wire exactly as Main.qml does: bridge relay -> service hydration.
    QObject::connect(&bridge, &PosEngineBridge::configUpdated, &config, &ConfigService::applyConfig);

    QSignalSpy configSpy(&bridge, &PosEngineBridge::configUpdated);

    // ── Connect: config hydrates over the Session stream. ──
    bridge.connectToEngine(QString::fromUtf8(target));
    ASSERT_TRUE(configSpy.wait(2000)) << "no ConfigUpdated on initial connect";
    EXPECT_TRUE(config.loaded());

    // ── Drop the session. ──
    bridge.disconnectFromEngine();
    ASSERT_FALSE(bridge.connected());
    configSpy.clear();

    // ── Re-establish: config must auto-rehydrate via the SAME Session path. ──
    bridge.connectToEngine(QString::fromUtf8(target));
    EXPECT_TRUE(configSpy.wait(2000)) << "config did not rehydrate after reconnect";
    EXPECT_TRUE(config.loaded());

    bridge.disconnectFromEngine();
}
