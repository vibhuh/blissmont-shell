// tests/unit/HeldCartModelTest.cpp — HeldCartModel full-reset-per-snapshot discipline (UX §10).
// Each ListHeldCarts returns a fresh HeldCartsList; the model replaces rows wholesale, so a
// shorter snapshot shrinks it and an empty one clears it (mirrors CartLineModelTest).
#include <gtest/gtest.h>

#include "models/HeldCartModel.hpp"
#include "terminal/v1/terminal.pb.h"

using blissmont::models::HeldCartModel;
namespace tv1 = blissmont::terminal::v1;

namespace {

tv1::HeldCartsList makeSnapshot(int count) {
    tv1::HeldCartsList snap;
    for (int i = 1; i <= count; ++i) {
        auto* h = snap.add_held_carts();
        h->set_held_cart_id("hold-" + std::to_string(i));
        h->set_label("counter " + std::to_string(i));
        h->set_held_at("2026-06-26T10:0" + std::to_string(i) + ":00Z");
        h->set_line_count(i);
        h->set_total_str("1" + std::to_string(i) + ".00");
    }
    return snap;
}

}  // namespace

TEST(HeldCartModel, ReflectsSnapshot) {
    HeldCartModel model;
    model.reset(makeSnapshot(2));
    EXPECT_EQ(model.rowCount(), 2);

    const auto idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, HeldCartModel::HeldCartIdRole).toString(), "hold-1");
    EXPECT_EQ(model.data(idx, HeldCartModel::LabelRole).toString(), "counter 1");
    EXPECT_EQ(model.data(idx, HeldCartModel::LineCountRole).toInt(), 1);
    EXPECT_EQ(model.data(idx, HeldCartModel::TotalRole).toString(), "11.00");
    EXPECT_EQ(model.data(idx, HeldCartModel::HeldAtRole).toString(), "2026-06-26T10:01:00Z");
}

TEST(HeldCartModel, FreshSnapshotReplacesRows) {
    HeldCartModel model;
    model.reset(makeSnapshot(3));
    EXPECT_EQ(model.rowCount(), 3);
    model.reset(makeSnapshot(1));
    EXPECT_EQ(model.rowCount(), 1);
    model.reset(tv1::HeldCartsList{});  // resume consumed the last hold → empty
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(HeldCartModel, ClearEmptiesTheModel) {
    HeldCartModel model;
    model.reset(makeSnapshot(2));
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(HeldCartModel, RoleNamesExposeBindings) {
    HeldCartModel model;
    const auto roles = model.roleNames();
    EXPECT_EQ(roles.value(HeldCartModel::HeldCartIdRole), "heldCartId");
    EXPECT_EQ(roles.value(HeldCartModel::LabelRole), "label");
    EXPECT_EQ(roles.value(HeldCartModel::LineCountRole), "lineCount");
    EXPECT_EQ(roles.value(HeldCartModel::TotalRole), "total");
    EXPECT_EQ(roles.value(HeldCartModel::HeldAtRole), "heldAt");
}
