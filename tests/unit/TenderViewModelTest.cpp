// tests/unit/TenderViewModelTest.cpp — tender logic, headless, no engine connection.
// The bridge is constructed but never connected, so command writes are safe no-ops; we
// drive the balance by feeding the bridge's CartSummary a CartUpdated snapshot directly
// (exactly the shape the engine pushes) and assert the VM's gating + completion logic.
#include <gtest/gtest.h>

#include <QVariantList>
#include <QVariantMap>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConfigService.hpp"
#include "viewmodels/TenderViewModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::bridge::PosEngineBridge;
using blissmont::services::ConfigService;
using blissmont::viewmodels::TenderViewModel;

namespace {

QVariantMap pm(const QString& method, const QString& referenceMode) {
    return QVariantMap{
        {QStringLiteral("method"), method},
        {QStringLiteral("displayName"), method.toUpper()},
        {QStringLiteral("hotkey"), QString()},
        {QStringLiteral("sortOrder"), 0},
        {QStringLiteral("enabled"), true},
        {QStringLiteral("referenceMode"), referenceMode},
    };
}

// Feed the bridge's CartSummary the balance the engine would have computed.
void setBalance(PosEngineBridge& bridge, const char* balanceDue) {
    blissmont::terminal::v1::CartUpdated snap;
    snap.set_balance_due_str(balanceDue);
    bridge.summary()->update(snap);
}

// Feed the bridge's CartSummary a bill total (as the engine emits it — full precision).
void setTotal(PosEngineBridge& bridge, const char* total) {
    blissmont::terminal::v1::CartUpdated snap;
    snap.set_total_str(total);
    bridge.summary()->update(snap);
}

}  // namespace

TEST(TenderViewModel, ReferenceModeForReadsConfig) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("upi"), QStringLiteral("required")),
                                    pm(QStringLiteral("cash"), QStringLiteral("none"))});

    EXPECT_EQ(tvm.referenceModeFor(QStringLiteral("upi")), QStringLiteral("required"));
    EXPECT_EQ(tvm.referenceModeFor(QStringLiteral("cash")), QStringLiteral("none"));
    EXPECT_EQ(tvm.referenceModeFor(QStringLiteral("unknown")), QStringLiteral("none"));
}

TEST(TenderViewModel, RequiredReferenceBlocksBlankTender) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);
    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("upi"), QStringLiteral("required"))});

    // Blank reference on a required method is blocked with a status message.
    tvm.addTender(QStringLiteral("upi"), QStringLiteral("100"), QStringLiteral("   "));
    EXPECT_FALSE(tvm.statusMessage().isEmpty());

    // A non-blank reference is allowed; status clears.
    tvm.addTender(QStringLiteral("upi"), QStringLiteral("100"), QStringLiteral("TXN-1"));
    EXPECT_TRUE(tvm.statusMessage().isEmpty());
}

TEST(TenderViewModel, OptionalAndNoneAllowBlankTender) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);
    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("cash"), QStringLiteral("none")),
                                    pm(QStringLiteral("upi"), QStringLiteral("optional"))});

    tvm.addTender(QStringLiteral("cash"), QStringLiteral("100"), QString());
    EXPECT_TRUE(tvm.statusMessage().isEmpty());
    tvm.addTender(QStringLiteral("upi"), QStringLiteral("100"), QString());
    EXPECT_TRUE(tvm.statusMessage().isEmpty());
}

TEST(TenderViewModel, CanCompleteOnlyWhenBalanceNonPositive) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    setBalance(bridge, "5.00");
    EXPECT_FALSE(tvm.canComplete());   // balance still due

    setBalance(bridge, "0.00");
    EXPECT_TRUE(tvm.canComplete());    // fully tendered

    setBalance(bridge, "-2.00");
    EXPECT_TRUE(tvm.canComplete());    // over-tendered (change due) still completable
}

TEST(TenderViewModel, AutoCompleteReflectsTenderCompleteMode) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"), {});
    EXPECT_FALSE(tvm.autoComplete());

    config.applyConfig(true, true, true, QStringLiteral("auto"), QStringLiteral("Rs"), {});
    EXPECT_TRUE(tvm.autoComplete());
}

TEST(TenderViewModel, PrimaryMethodPrefersCashThenFirst) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    // No config yet → the cash default.
    EXPECT_EQ(tvm.primaryMethod(), QStringLiteral("cash"));

    // Cash present (not first) → cash wins.
    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("upi"), QStringLiteral("required")),
                                    pm(QStringLiteral("cash"), QStringLiteral("none"))});
    EXPECT_EQ(tvm.primaryMethod(), QStringLiteral("cash"));

    // No cash → the first enabled method.
    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("card"), QStringLiteral("none")),
                                    pm(QStringLiteral("upi"), QStringLiteral("required"))});
    EXPECT_EQ(tvm.primaryMethod(), QStringLiteral("card"));
}

TEST(TenderViewModel, ChangeDuePreviewIsExactAndClamped) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    // Engine emits the total at full precision — the preview must still parse it.
    setTotal(bridge, "1003.0000000000");

    EXPECT_EQ(tvm.changeDuePreview(QStringLiteral("1003")), QStringLiteral("0.00"));   // exact
    EXPECT_EQ(tvm.changeDuePreview(QStringLiteral("1500")), QStringLiteral("497.00")); // over
    EXPECT_EQ(tvm.changeDuePreview(QStringLiteral("500")), QStringLiteral("0.00"));    // short → clamped
    EXPECT_EQ(tvm.changeDuePreview(QStringLiteral("")), QStringLiteral("0.00"));       // empty
    // Mid-keystroke value with >2 dp is rounded before the exact subtraction.
    EXPECT_EQ(tvm.changeDuePreview(QStringLiteral("1003.567")), QStringLiteral("0.57"));
}

TEST(TenderViewModel, TenderAndCompleteBlocksOnRequiredBlankReference) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);
    config.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"),
                       QVariantList{pm(QStringLiteral("upi"), QStringLiteral("required"))});

    // Required reference, blank → blocked with a status message (and not armed/sent).
    tvm.tenderAndComplete(QStringLiteral("upi"), QStringLiteral("100"), QStringLiteral("  "));
    EXPECT_FALSE(tvm.statusMessage().isEmpty());

    // With a reference it dispatches and clears the status.
    tvm.tenderAndComplete(QStringLiteral("upi"), QStringLiteral("100"), QStringLiteral("TXN-9"));
    EXPECT_TRUE(tvm.statusMessage().isEmpty());
}

TEST(TenderViewModel, StateChangesWhenBalanceUpdates) {
    PosEngineBridge bridge;
    ConfigService config;
    TenderViewModel tvm;
    tvm.setBridge(&bridge);
    tvm.setConfig(&config);

    int stateChanges = 0;
    QObject::connect(&tvm, &TenderViewModel::stateChanged, [&stateChanges] { ++stateChanges; });
    setBalance(bridge, "0.00");  // a fresh snapshot must re-drive canComplete
    EXPECT_GE(stateChanges, 1);
}
