// tests/unit/PayoutViewModelTest.cpp — payout-panel logic, headless, no engine.
// The bridge is constructed but never connected, so command writes are safe no-ops; we drive
// the panel state by emitting the bridge's payoutRecorded / commandRejected signals (what
// applyEvent does on the corresponding events), then assert the VM's status + recorded signal.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "viewmodels/PayoutViewModel.hpp"

using blissmont::bridge::PosEngineBridge;
using blissmont::viewmodels::PayoutViewModel;

TEST(PayoutViewModel, DispatchersAreSafeWithoutAConnection) {
    PayoutViewModel pvm;  // no bridge set
    pvm.recordPayout(QStringLiteral("100.00"), QStringLiteral("supplier"));
    EXPECT_TRUE(pvm.statusMessage().isEmpty());  // no-op, no crash

    PosEngineBridge bridge;  // constructed, never connected
    pvm.setBridge(&bridge);
    pvm.recordPayout(QStringLiteral("100.00"), QStringLiteral("supplier"));  // safe write no-op
    SUCCEED();
}

TEST(PayoutViewModel, RequiresAmountAndCategoryBeforeDispatch) {
    PosEngineBridge bridge;
    PayoutViewModel pvm;
    pvm.setBridge(&bridge);

    // Missing category — gated, surfaces a hint, does not dispatch.
    pvm.recordPayout(QStringLiteral("100.00"), QString());
    EXPECT_FALSE(pvm.statusMessage().isEmpty());

    // Missing amount — same gate.
    pvm.recordPayout(QString(), QStringLiteral("supplier"));
    EXPECT_FALSE(pvm.statusMessage().isEmpty());
}

TEST(PayoutViewModel, PayoutRecordedSurfacesConfirmationAndFiresRecorded) {
    PosEngineBridge bridge;
    PayoutViewModel pvm;
    pvm.setBridge(&bridge);

    QSignalSpy recorded(&pvm, &PayoutViewModel::recorded);
    QSignalSpy statusChanged(&pvm, &PayoutViewModel::statusMessageChanged);

    // What applyEvent emits on a kPayoutRecorded event from the engine.
    emit bridge.payoutRecorded(QStringLiteral("payout-1"), QStringLiteral("250.00"),
                               QStringLiteral("petty_cash"));

    ASSERT_EQ(recorded.count(), 1);
    EXPECT_EQ(recorded.at(0).at(0).toString(), QStringLiteral("250.00"));
    EXPECT_EQ(recorded.at(0).at(1).toString(), QStringLiteral("petty_cash"));
    EXPECT_TRUE(pvm.statusMessage().contains(QStringLiteral("250.00")));
    EXPECT_TRUE(pvm.statusMessage().contains(QStringLiteral("petty_cash")));
    EXPECT_GE(statusChanged.count(), 1);
}

TEST(PayoutViewModel, CommandRejectedSurfacesVerbatim) {
    PosEngineBridge bridge;
    PayoutViewModel pvm;
    pvm.setBridge(&bridge);

    // An engine reject (e.g. unknown/disabled category) surfaces as status.
    emit bridge.commandRejected(QStringLiteral("UNKNOWN_CATEGORY"),
                                QStringLiteral("Unknown or disabled payout category"));
    EXPECT_EQ(pvm.statusMessage(), QStringLiteral("Unknown or disabled payout category"));

    // Empty message falls back to the code.
    emit bridge.commandRejected(QStringLiteral("NO_PERIOD"), QString());
    EXPECT_EQ(pvm.statusMessage(), QStringLiteral("NO_PERIOD"));
}
