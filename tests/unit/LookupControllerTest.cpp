// tests/unit/LookupControllerTest.cpp — the ranked keyboard-lookup engine, headless.
// Proves the two things the brief makes load-bearing and keeps in C++ for exactly this reason:
//   1) the ranked filter order — exact > starts-with > word-starts-with > contains (stable on
//      ties), so "app" puts "Apple Juice" above "Pineapple";
//   2) the Enter-selection PRIORITY — exact → single → highlighted → none, never needing Down.
// Plus scope, highlight reset, and navigation clamping. No QML, no display.
#include <gtest/gtest.h>

#include <QVariantList>
#include <QVariantMap>

#include "viewmodels/LookupController.hpp"

using blissmont::viewmodels::LookupController;

namespace {

QVariantMap item(const QString& name, const QString& sku = QString(),
                 const QString& barcode = QString()) {
    return QVariantMap{{QStringLiteral("name"), name},
                       {QStringLiteral("sku"), sku},
                       {QStringLiteral("barcode"), barcode}};
}

QString nameAt(const LookupController& c, int row) {
    return c.itemAt(row).value(QStringLiteral("name")).toString();
}

}  // namespace

// ── Ranking: exact > starts-with > word-starts-with > contains ─────────────────
TEST(LookupController, RanksByMatchQualityBestFirst) {
    LookupController c;
    c.setItems(QVariantList{
        item("Pineapple"),     // contains "app"
        item("Green Apple"),   // word-starts: "Apple" begins with "app"
        item("app"),           // exact
        item("Apple Juice"),   // starts-with
    });
    c.setSearchText("app");

    ASSERT_EQ(c.count(), 4);
    EXPECT_EQ(nameAt(c, 0), "app");           // exact
    EXPECT_EQ(nameAt(c, 1), "Apple Juice");   // starts-with
    EXPECT_EQ(nameAt(c, 2), "Green Apple");   // word-starts-with
    EXPECT_EQ(nameAt(c, 3), "Pineapple");     // contains
    EXPECT_EQ(c.currentIndex(), 0);           // best match highlighted from the filter
}

TEST(LookupController, StartsWithBeatsContains_TheBriefExample) {
    LookupController c;
    c.setItems(QVariantList{item("Pineapple"), item("Apple Juice")});
    c.setSearchText("app");
    EXPECT_EQ(nameAt(c, 0), "Apple Juice");   // starts-with ranks ABOVE contains
}

TEST(LookupController, StableWithinTierPreservesInputOrder) {
    LookupController c;
    c.setItems(QVariantList{item("Apple A"), item("Apple B"), item("Apple C")});
    c.setSearchText("apple");  // all starts-with → input order preserved
    EXPECT_EQ(nameAt(c, 0), "Apple A");
    EXPECT_EQ(nameAt(c, 1), "Apple B");
    EXPECT_EQ(nameAt(c, 2), "Apple C");
}

TEST(LookupController, NonMatchesAreDropped) {
    LookupController c;
    c.setItems(QVariantList{item("Apple"), item("Banana"), item("Grape")});
    c.setSearchText("an");          // Banana (word? no — contains), Grape? no. Apple? no.
    EXPECT_EQ(c.count(), 1);
    EXPECT_EQ(nameAt(c, 0), "Banana");
}

// ── Enter-selection priority ──────────────────────────────────────────────────
TEST(LookupController, SelectionPrefersExactMatch) {
    LookupController c;
    c.setItems(QVariantList{item("Milk Powder"), item("Soy Milk"), item("Milk")});
    c.setSearchText("milk");
    EXPECT_EQ(c.resolveSelectionIndex(), 0);                  // exact "Milk" sorted to row 0
    EXPECT_EQ(nameAt(c, c.resolveSelectionIndex()), "Milk");
}

TEST(LookupController, SelectionFallsToSingleResult) {
    LookupController c;
    c.setItems(QVariantList{item("Milk Powder"), item("Soy Milk")});
    c.setSearchText("powder");                                // only one match, not exact
    EXPECT_EQ(c.count(), 1);
    EXPECT_EQ(c.resolveSelectionIndex(), 0);
}

TEST(LookupController, SelectionFallsToHighlightedWhenMultipleAndNoExact) {
    LookupController c;
    c.setItems(QVariantList{item("Milk Powder"), item("Milky Bar"), item("Soy Milk")});
    c.setSearchText("mi");                                    // multiple, no exact
    ASSERT_GT(c.count(), 1);
    EXPECT_EQ(c.resolveSelectionIndex(), 0);                  // highlight starts at best match
    c.moveDown();
    EXPECT_EQ(c.currentIndex(), 1);
    EXPECT_EQ(c.resolveSelectionIndex(), 1);                  // follows the highlight
}

TEST(LookupController, NoSelectionWhenQueryEmptyOrNoMatch) {
    LookupController c;
    c.setItems(QVariantList{item("Apple"), item("Banana")});
    EXPECT_EQ(c.resolveSelectionIndex(), -1);                 // empty query → no implicit pick
    EXPECT_TRUE(c.selectedItem().isEmpty());
    c.setSearchText("zzz");                                   // no match
    EXPECT_EQ(c.count(), 0);
    EXPECT_EQ(c.resolveSelectionIndex(), -1);
    EXPECT_EQ(c.currentIndex(), -1);
}

// ── Scope ─────────────────────────────────────────────────────────────────────
TEST(LookupController, BarcodeScopeMatchesOnlyBarcodeKey) {
    LookupController c;
    c.setSearchKeys(QStringList{QStringLiteral("name"), QStringLiteral("sku")});
    c.setBarcodeKey(QStringLiteral("barcode"));
    c.setItems(QVariantList{item("Toor Dal", "TD1", "8901001"),
                            item("Rice", "BR5", "8901002")});

    c.setScope(QStringLiteral("all"));
    c.setSearchText("8901001");                 // not in name/sku → no match in "all"
    EXPECT_EQ(c.count(), 0);

    c.setScope(QStringLiteral("barcode"));      // now the indexed barcode column matches
    EXPECT_EQ(c.count(), 1);
    EXPECT_EQ(nameAt(c, 0), "Toor Dal");
}

TEST(LookupController, AllScopeSearchesEveryKey) {
    LookupController c;
    c.setSearchKeys(QStringList{QStringLiteral("name"), QStringLiteral("sku")});
    c.setItems(QVariantList{item("Toor Dal", "TD1"), item("Rice", "BR5")});
    c.setSearchText("td1");                      // SKU match in "all" scope
    EXPECT_EQ(c.count(), 1);
    EXPECT_EQ(nameAt(c, 0), "Toor Dal");
}

// ── Highlight is list-state; navigation clamps ────────────────────────────────
TEST(LookupController, RefilterResetsHighlightToBestMatch) {
    LookupController c;
    c.setItems(QVariantList{item("Milk Powder"), item("Milky Bar"), item("Soy Milk")});
    c.setSearchText("mi");
    c.moveEnd();
    EXPECT_GT(c.currentIndex(), 0);
    c.setSearchText("mil");                      // re-filter → highlight back to row 0
    EXPECT_EQ(c.currentIndex(), 0);
}

TEST(LookupController, NavigationClampsToBounds) {
    LookupController c;
    c.setItems(QVariantList{item("A pick"), item("B pick"), item("C pick")});
    c.setSearchText("pick");
    ASSERT_EQ(c.count(), 3);

    c.moveUp();                                  // already at 0
    EXPECT_EQ(c.currentIndex(), 0);
    c.moveEnd();
    EXPECT_EQ(c.currentIndex(), 2);
    c.moveDown();                                // past the end clamps
    EXPECT_EQ(c.currentIndex(), 2);
    c.moveHome();
    EXPECT_EQ(c.currentIndex(), 0);
    c.movePage(100);
    EXPECT_EQ(c.currentIndex(), 2);
    c.movePage(-100);
    EXPECT_EQ(c.currentIndex(), 0);
}

TEST(LookupController, SetItemsResetsHighlight) {
    LookupController c;
    c.setItems(QVariantList{item("A pick"), item("B pick")});
    c.setSearchText("pick");
    c.moveEnd();
    EXPECT_EQ(c.currentIndex(), 1);
    c.setItems(QVariantList{item("X pick"), item("Y pick"), item("Z pick")});  // search retained
    EXPECT_EQ(c.count(), 3);
    EXPECT_EQ(c.currentIndex(), 0);              // highlight reset on new items
}

TEST(LookupController, RoleNamesExposeItem) {
    LookupController c;
    EXPECT_EQ(c.roleNames().value(LookupController::ItemRole), "item");
}
