// tests/unit/BillingViewModelTest.cpp — view-model intent, headless, no engine connection.
// The bridge is constructed but never connected (connectToEngine is not called), so command
// writes are safe no-ops — we assert on the VM's view-only state transitions.
#include <gtest/gtest.h>

#include "bridge/PosEngineBridge.hpp"
#include "viewmodels/BillingViewModel.hpp"

using blissmont::bridge::PosEngineBridge;
using blissmont::viewmodels::BillingViewModel;

TEST(BillingViewModel, SubmitScanClearsTextForHomeReturn) {
    PosEngineBridge bridge;          // not connected — writes are no-ops
    BillingViewModel vm;
    vm.setBridge(&bridge);

    vm.setScanText("ABC123");
    EXPECT_EQ(vm.scanText(), "ABC123");
    vm.submitScan();
    EXPECT_TRUE(vm.scanText().isEmpty());  // cleared -> focus returns to empty scan field
}

TEST(BillingViewModel, BlankSubmitIsNoOp) {
    PosEngineBridge bridge;
    BillingViewModel vm;
    vm.setBridge(&bridge);

    vm.setScanText("   ");
    vm.submitScan();  // whitespace-only -> ignored, no crash
    EXPECT_EQ(vm.statusMessage(), "");
}

TEST(BillingViewModel, BridgeAssignmentIsIdempotent) {
    PosEngineBridge bridge;
    BillingViewModel vm;
    int changes = 0;
    QObject::connect(&vm, &BillingViewModel::bridgeChanged, [&changes] { ++changes; });
    vm.setBridge(&bridge);
    vm.setBridge(&bridge);  // same pointer -> no second signal
    EXPECT_EQ(changes, 1);
}
