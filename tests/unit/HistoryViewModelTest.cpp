// tests/unit/HistoryViewModelTest.cpp — history logic, headless, no engine connection.
// The bridge is constructed but never connected, so command writes are safe no-ops; we drive
// the panel state by feeding the bridge's BillDetailModel a BillDetail snapshot directly (what
// the bridge's reader does on kBillDetail) and assert the VM's list↔detail toggle, reprint
// intent handling, and the offline hint.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConnectionService.hpp"
#include "viewmodels/HistoryViewModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::bridge::PosEngineBridge;
using blissmont::services::ConnectionService;
using blissmont::viewmodels::HistoryViewModel;
namespace tv1 = blissmont::terminal::v1;

namespace {

// A BillDetail as the engine would push it on recall / reprint.
tv1::BillDetail makeDetail(const char* receiptNo) {
    tv1::BillDetail d;
    d.set_receipt_no(receiptNo);
    d.set_status("settled");
    d.set_subtotal_str("200.00");
    d.set_tax_total_str("36.00");
    d.set_total_str("236.00");
    d.set_customer_label("Walk-in");
    auto* l = d.add_lines();
    l->set_line_no(1);
    l->set_description("Widget");
    l->set_qty_str("1");
    l->set_line_total_str("236.00");
    auto* p = d.add_payments();
    p->set_tender_no(1);
    p->set_method("cash");
    p->set_amount_str("236.00");
    return d;
}

// Feed the bridge's BillDetailModel a snapshot (what applyEvent does on kBillDetail).
void loadDetail(PosEngineBridge& bridge, const char* receiptNo) {
    bridge.billDetail()->reset(makeDetail(receiptNo));
}

}  // namespace

TEST(HistoryViewModel, DetailActiveTracksTheDetailModel) {
    PosEngineBridge bridge;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);

    EXPECT_FALSE(hvm.detailActive());

    loadDetail(bridge, "S01-P000001");  // engine pushed a recalled bill
    EXPECT_TRUE(hvm.detailActive());

    hvm.closeDetail();                  // back to the list
    EXPECT_FALSE(hvm.detailActive());
}

TEST(HistoryViewModel, ReprintFromListStaysOnListWithDuplicateStatus) {
    PosEngineBridge bridge;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);

    // Reprint a row from the recent list (no detail showing).
    EXPECT_FALSE(hvm.detailActive());
    hvm.reprint(QStringLiteral("S01-P000007"));

    // The engine echoes a BillDetail after printing; it must NOT navigate into detail.
    loadDetail(bridge, "S01-P000007");
    EXPECT_FALSE(hvm.detailActive()) << "a list reprint must not open the detail view";
    EXPECT_TRUE(hvm.statusMessage().contains(QStringLiteral("DUPLICATE")));
}

TEST(HistoryViewModel, ReprintFromDetailStaysInDetail) {
    PosEngineBridge bridge;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);

    // View a bill's detail first.
    loadDetail(bridge, "S01-P000002");
    EXPECT_TRUE(hvm.detailActive());

    // Reprint from inside detail: the echoed BillDetail keeps the detail in view.
    hvm.reprint(QStringLiteral("S01-P000002"));
    loadDetail(bridge, "S01-P000002");
    EXPECT_TRUE(hvm.detailActive()) << "a detail reprint must keep the detail in view";
    EXPECT_TRUE(hvm.statusMessage().contains(QStringLiteral("DUPLICATE")));
}

TEST(HistoryViewModel, StartReturnRequestsNavigation) {
    PosEngineBridge bridge;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);

    QSignalSpy navSpy(&hvm, &HistoryViewModel::returnRequested);
    hvm.startReturn(QStringLiteral("S01-P000003"));
    EXPECT_EQ(navSpy.count(), 1) << "starting a return must ask the host to switch context";
}

TEST(HistoryViewModel, LocalOnlyHintTracksEngineOnline) {
    PosEngineBridge bridge;
    ConnectionService conn;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);
    hvm.setConnection(&conn);

    conn.applySyncStatus(/*online=*/false, /*pending=*/0);
    EXPECT_TRUE(hvm.localOnlyHint());

    conn.applySyncStatus(/*online=*/true, /*pending=*/0);
    EXPECT_FALSE(hvm.localOnlyHint());
}

TEST(HistoryViewModel, OpenClearsAnyShowingDetail) {
    PosEngineBridge bridge;
    HistoryViewModel hvm;
    hvm.setBridge(&bridge);

    loadDetail(bridge, "S01-P000004");
    EXPECT_TRUE(hvm.detailActive());

    hvm.open();  // re-open the panel → recent-list-first, detail dropped
    EXPECT_FALSE(hvm.detailActive());
}
