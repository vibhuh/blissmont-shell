// tests/unit/SuspendResumeViewModelTest.cpp — suspend/resume logic, headless, no engine.
// The bridge is constructed but never connected, so command writes are safe no-ops; we drive
// the panel state by feeding the bridge's HeldCartModel a HeldCartsList snapshot directly (what
// the bridge's reader does on kHeldCartsList) and by emitting the bridge's cartHeld/
// commandRejected signals (what applyEvent does), then assert the VM's holds state and status.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "viewmodels/SuspendResumeViewModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::bridge::PosEngineBridge;
using blissmont::viewmodels::SuspendResumeViewModel;
namespace tv1 = blissmont::terminal::v1;

namespace {

// A HeldCartsList as the engine would push it on ListHeldCarts.
tv1::HeldCartsList makeHolds(int count) {
    tv1::HeldCartsList snap;
    for (int i = 1; i <= count; ++i) {
        auto* h = snap.add_held_carts();
        h->set_held_cart_id("hold-" + std::to_string(i));
        h->set_label("counter " + std::to_string(i));
        h->set_line_count(i);
        h->set_total_str("100.00");
    }
    return snap;
}

// Feed the bridge's HeldCartModel a snapshot (what applyEvent does on kHeldCartsList).
void loadHolds(PosEngineBridge& bridge, int count) {
    bridge.heldCarts()->reset(makeHolds(count));
}

}  // namespace

TEST(SuspendResumeViewModel, NoHoldsUntilListed) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    EXPECT_FALSE(srm.hasHolds());

    loadHolds(bridge, 2);  // engine replied HeldCartsList
    EXPECT_TRUE(srm.hasHolds());
}

TEST(SuspendResumeViewModel, ListedHoldsEmitHoldsChanged) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    QSignalSpy spy(&srm, &SuspendResumeViewModel::holdsChanged);
    loadHolds(bridge, 1);
    EXPECT_GE(spy.count(), 1);
}

TEST(SuspendResumeViewModel, CartHeldConfirmationUsesLabel) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    // applyEvent emits cartHeld on kCartHeld; emulate it (signals are public in Qt).
    emit bridge.cartHeld(QStringLiteral("019e-uuid-7"), QStringLiteral("counter 2"));
    EXPECT_TRUE(srm.statusMessage().contains(QStringLiteral("counter 2")));
}

TEST(SuspendResumeViewModel, CartHeldConfirmationFallsBackToShortId) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    emit bridge.cartHeld(QStringLiteral("019e14f2-9cfa-7f70"), QString());
    // No label → the slip is identified by a short id prefix.
    EXPECT_TRUE(srm.statusMessage().contains(QStringLiteral("019e14f2")));
}

TEST(SuspendResumeViewModel, RejectSurfacesAsStatus) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    // e.g. holding an empty cart — the engine rejects EMPTY_CART.
    emit bridge.commandRejected(QStringLiteral("EMPTY_CART"), QStringLiteral("Nothing to hold"));
    EXPECT_EQ(srm.statusMessage(), QStringLiteral("Nothing to hold"));
}

TEST(SuspendResumeViewModel, DispatchersAreSafeWithoutAConnection) {
    PosEngineBridge bridge;
    SuspendResumeViewModel srm;
    srm.setBridge(&bridge);

    // Unconnected bridge → writes are no-ops; the VM must not crash or block.
    srm.hold(QStringLiteral("counter 2"));
    srm.openResume();
    srm.resume(QStringLiteral("hold-1"));
    srm.resume(QString());  // empty id ignored
    SUCCEED();
}
