// tests/unit/CartLineModelTest.cpp — the model reflects a CartUpdated snapshot exactly,
// and a fresh snapshot fully replaces prior rows (no stale lines). Headless QAbstractListModel.
#include <gtest/gtest.h>

#include "models/CartLineModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::models::CartLineModel;
namespace tv1 = blissmont::terminal::v1;

namespace {

tv1::CartUpdated makeSnapshot(int lineCount) {
    tv1::CartUpdated snap;
    for (int i = 1; i <= lineCount; ++i) {
        auto* l = snap.add_lines();
        l->set_line_no(i);
        l->set_sku("SKU" + std::to_string(i));
        l->set_description("Item " + std::to_string(i));
        l->set_qty_str("1");
        l->set_unit_price_str("10.00");
        l->set_line_total_str("10.00");
    }
    return snap;
}

}  // namespace

TEST(CartLineModel, ReflectsSnapshot) {
    CartLineModel model;
    model.reset(makeSnapshot(3));
    EXPECT_EQ(model.rowCount(), 3);

    const auto idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, CartLineModel::SkuRole).toString(), "SKU1");
    EXPECT_EQ(model.data(idx, CartLineModel::DescriptionRole).toString(), "Item 1");
    EXPECT_EQ(model.data(idx, CartLineModel::LineTotalRole).toString(), "10.00");
}

TEST(CartLineModel, FreshSnapshotReplacesRows) {
    CartLineModel model;
    model.reset(makeSnapshot(5));
    EXPECT_EQ(model.rowCount(), 5);
    model.reset(makeSnapshot(2));  // engine pushed a smaller cart
    EXPECT_EQ(model.rowCount(), 2);
    model.reset(tv1::CartUpdated{});  // emptied
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(CartLineModel, RoleNamesExposeBindings) {
    CartLineModel model;
    const auto roles = model.roleNames();
    EXPECT_EQ(roles.value(CartLineModel::QtyRole), "qty");
    EXPECT_EQ(roles.value(CartLineModel::UnitPriceRole), "unitPrice");
}
