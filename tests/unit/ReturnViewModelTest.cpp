// tests/unit/ReturnViewModelTest.cpp — returns logic, headless, no engine connection.
// The bridge is constructed but never connected, so command writes are safe no-ops; we
// drive the return context by feeding the bridge's ReturnLineModel a ReturnContextLoaded
// snapshot directly (exactly the shape the engine pushes, and what the bridge's reader does
// on kReturnContextLoaded) and assert the VM's gating + completion logic.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConfigService.hpp"
#include "viewmodels/ReturnViewModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::bridge::PosEngineBridge;
using blissmont::services::ConfigService;
using blissmont::viewmodels::ReturnViewModel;

namespace {

// Apply the five returns axes onto a config (defaults: cash/no-blind/partial-on).
void applyReturns(ConfigService& cfg, const QString& refundTenderMode,
                  bool allowBlindReturn = false) {
    cfg.applyConfig(/*allowReturns=*/true, /*payoutEnabled=*/true, /*allowDiscounts=*/true,
                    QStringLiteral("confirm"), QStringLiteral("Rs"), {}, allowBlindReturn,
                    refundTenderMode, QStringLiteral("never"), /*restockDefault=*/true,
                    /*allowPartialReturn=*/true);
}

// One returnable line as ReturnContextLoaded would carry it.
void addLine(blissmont::terminal::v1::ReturnContextLoaded& ctx, int lineNo,
             const char* selectedQty) {
    auto* l = ctx.add_lines();
    l->set_original_line_no(lineNo);
    l->set_description("Widget");
    l->set_sold_qty_str("2.0000000000");
    l->set_refundable_qty_str("2.0000000000");
    l->set_selected_qty_str(selectedQty);
    l->set_unit_price_str("100.0000000000");
    l->set_line_total_str("200.0000000000");
    l->set_tax_amount_str("36.0000000000");
    l->set_restock(true);
}

// Feed the bridge's ReturnLineModel a context (what applyEvent does on kReturnContextLoaded).
void loadContext(PosEngineBridge& bridge, const char* selectedQty) {
    blissmont::terminal::v1::ReturnContextLoaded ctx;
    ctx.set_original_receipt_no("S01-P000001");
    addLine(ctx, 1, selectedQty);
    bridge.returnLines()->reset(ctx);
}

}  // namespace

TEST(ReturnViewModel, NotActiveUntilContextLoaded) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);
    applyReturns(config, QStringLiteral("cash"));

    EXPECT_FALSE(rvm.active());
    EXPECT_FALSE(rvm.canCommit());

    loadContext(bridge, "2.0000000000");
    EXPECT_TRUE(rvm.active());
}

TEST(ReturnViewModel, CanCommitOnlyWithASelectedLine) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);
    applyReturns(config, QStringLiteral("cash"));

    // A loaded context whose only line is selected 0 cannot be committed.
    loadContext(bridge, "0.0000000000");
    EXPECT_TRUE(rvm.active());
    EXPECT_FALSE(rvm.canCommit());

    // A positive selected qty enables commit.
    loadContext(bridge, "1.0000000000");
    EXPECT_TRUE(rvm.canCommit());
}

TEST(ReturnViewModel, BothRefundModeIsUnsupportedAndBlocksCommit) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);
    applyReturns(config, QStringLiteral("both"));  // split refund — Phase A unsupported
    loadContext(bridge, "1.0000000000");

    EXPECT_FALSE(rvm.refundModeSupported());
    EXPECT_FALSE(rvm.canCommit());  // disabled even with a selected line

    // commit() is blocked with a status message and sends nothing.
    rvm.commit();
    EXPECT_FALSE(rvm.statusMessage().isEmpty());
}

TEST(ReturnViewModel, CashAndOriginalModesAreSupported) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);

    applyReturns(config, QStringLiteral("cash"));
    EXPECT_TRUE(rvm.refundModeSupported());
    applyReturns(config, QStringLiteral("original"));
    EXPECT_TRUE(rvm.refundModeSupported());
}

TEST(ReturnViewModel, BlindReturnGatedByConfig) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);

    // Blind disabled: a blind start is blocked with a status, nothing dispatched.
    applyReturns(config, QStringLiteral("cash"), /*allowBlindReturn=*/false);
    EXPECT_FALSE(rvm.allowBlind());
    rvm.startReturn(QString(), /*blind=*/true);
    EXPECT_FALSE(rvm.statusMessage().isEmpty());

    // Blind enabled: a blind start is allowed and clears the status.
    applyReturns(config, QStringLiteral("cash"), /*allowBlindReturn=*/true);
    EXPECT_TRUE(rvm.allowBlind());
    rvm.startReturn(QStringLiteral("S01-P000001"), /*blind=*/true);
    EXPECT_TRUE(rvm.statusMessage().isEmpty());
}

TEST(ReturnViewModel, ContextResetReDrivesState) {
    PosEngineBridge bridge;
    ConfigService config;
    ReturnViewModel rvm;
    rvm.setBridge(&bridge);
    rvm.setConfig(&config);
    applyReturns(config, QStringLiteral("cash"));

    int stateChanges = 0;
    QObject::connect(&rvm, &ReturnViewModel::stateChanged, [&stateChanges] { ++stateChanges; });
    loadContext(bridge, "1.0000000000");  // a fresh snapshot must re-drive canCommit
    EXPECT_GE(stateChanges, 1);
}
