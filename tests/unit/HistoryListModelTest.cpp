// tests/unit/HistoryListModelTest.cpp — the model reflects a HistoryResults snapshot exactly,
// and a fresh snapshot fully replaces prior rows (no stale bills). Headless QAbstractListModel.
#include <gtest/gtest.h>

#include "models/HistoryListModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::models::HistoryListModel;
namespace tv1 = blissmont::terminal::v1;

namespace {

tv1::HistoryResults makeResults(int billCount) {
    tv1::HistoryResults res;
    for (int i = 1; i <= billCount; ++i) {
        auto* b = res.add_bills();
        b->set_receipt_no("S01-P00000" + std::to_string(i));
        b->set_order_id("ord-" + std::to_string(i));
        b->set_status("settled");
        b->set_total_str("100.00");
        b->set_settled_at("2026-06-26T10:0" + std::to_string(i) + ":00Z");
        b->set_customer_label("Walk-in");
        b->set_provisional(i % 2 == 0);  // every other one provisional
    }
    return res;
}

}  // namespace

TEST(HistoryListModel, ReflectsSnapshot) {
    HistoryListModel model;
    model.reset(makeResults(3));
    EXPECT_EQ(model.rowCount(), 3);

    const auto idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, HistoryListModel::ReceiptNoRole).toString(), "S01-P000001");
    EXPECT_EQ(model.data(idx, HistoryListModel::TotalRole).toString(), "100.00");
    EXPECT_EQ(model.data(idx, HistoryListModel::CustomerLabelRole).toString(), "Walk-in");
    EXPECT_FALSE(model.data(idx, HistoryListModel::ProvisionalRole).toBool());
    EXPECT_TRUE(model.data(model.index(1, 0), HistoryListModel::ProvisionalRole).toBool());
}

TEST(HistoryListModel, FreshSnapshotReplacesRows) {
    HistoryListModel model;
    model.reset(makeResults(5));
    EXPECT_EQ(model.rowCount(), 5);
    model.reset(makeResults(2));  // a narrower search result set
    EXPECT_EQ(model.rowCount(), 2);
    model.clear();                // panel re-opened / search cleared
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(HistoryListModel, RoleNamesExposeBindings) {
    HistoryListModel model;
    const auto roles = model.roleNames();
    EXPECT_EQ(roles.value(HistoryListModel::ReceiptNoRole), "receiptNo");
    EXPECT_EQ(roles.value(HistoryListModel::SettledAtRole), "settledAt");
    EXPECT_EQ(roles.value(HistoryListModel::ProvisionalRole), "provisional");
}
