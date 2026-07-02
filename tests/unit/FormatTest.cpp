// tests/unit/FormatTest.cpp — the ONE presentation-formatting standard (core/numfmt).
// Pure logic, no Qt. The bug this guards: raw engine precision ("1003.0000000000",
// "1.0000000000") must never reach the cashier's eye (refinement brief, Phase 1).
#include <gtest/gtest.h>

#include "core/Format.hpp"

namespace nf = blissmont::core::numfmt;

TEST(FormatMoney, StripsRawPrecisionToTwoDecimals) {
    EXPECT_EQ(nf::money("1003.0000000000"), "1,003.00");  // the reported defect
    EXPECT_EQ(nf::money("531.0000000000"), "531.00");
    EXPECT_EQ(nf::money("40.5"), "40.50");
    EXPECT_EQ(nf::money("1003"), "1,003.00");
    EXPECT_EQ(nf::money("0"), "0.00");
}

TEST(FormatMoney, EmptyAndJunkAreZero) {
    EXPECT_EQ(nf::money(""), "0.00");
    EXPECT_EQ(nf::money("abc"), "0.00");
    EXPECT_EQ(nf::money("1.2.3"), "0.00");
}

TEST(FormatMoney, HalfUpRounding) {
    EXPECT_EQ(nf::money("1.005"), "1.01");
    EXPECT_EQ(nf::money("1.004"), "1.00");
    EXPECT_EQ(nf::money("0.999"), "1.00");      // carry into integer
    EXPECT_EQ(nf::money("99.999"), "100.00");   // carry growing the integer width
}

TEST(FormatMoney, IndianGrouping) {
    EXPECT_EQ(nf::money("100000"), "1,00,000.00");      // one lakh
    EXPECT_EQ(nf::money("10000000"), "1,00,00,000.00");  // one crore
    EXPECT_EQ(nf::money("12345.6"), "12,345.60");
    EXPECT_EQ(nf::money("999.99"), "999.99");
}

TEST(FormatMoney, Negative) {
    EXPECT_EQ(nf::money("-5"), "-5.00");
    EXPECT_EQ(nf::money("-100000.5"), "-1,00,000.50");
    EXPECT_EQ(nf::money("-0.001"), "0.00");  // rounds to zero, no "-0.00"
}

TEST(FormatQty, WholeNumbersRenderBare) {
    EXPECT_EQ(nf::qty("1.0000000000"), "1");  // the reported defect
    EXPECT_EQ(nf::qty("1"), "1");
    EXPECT_EQ(nf::qty("12.000"), "12");
    EXPECT_EQ(nf::qty("0"), "0");
}

TEST(FormatQty, FractionalTrimsTrailingZeros) {
    EXPECT_EQ(nf::qty("1.2500000000"), "1.25");
    EXPECT_EQ(nf::qty("0.750"), "0.75");
    EXPECT_EQ(nf::qty("2.500"), "2.5");
    EXPECT_EQ(nf::qty("1.235"), "1.235");
}

TEST(FormatQty, RespectsPrecisionCap) {
    EXPECT_EQ(nf::qty("1.2345", 3), "1.235");  // half-up at the cap
    EXPECT_EQ(nf::qty("1.2344", 3), "1.234");
    EXPECT_EQ(nf::qty("1.5", 0), "2");          // 0 precision -> whole, half-up
}

TEST(FormatPercent, FixedTwoDecimals) {
    EXPECT_EQ(nf::percent("18"), "18.00%");
    EXPECT_EQ(nf::percent("5"), "5.00%");
    EXPECT_EQ(nf::percent("2.5"), "2.50%");
}

TEST(FormatFractionAsPercent, TaxRateFractionToPercent) {
    EXPECT_EQ(nf::fractionAsPercent("0.05"), "5.00%");
    EXPECT_EQ(nf::fractionAsPercent("0.18"), "18.00%");
    EXPECT_EQ(nf::fractionAsPercent("0.12"), "12.00%");
    EXPECT_EQ(nf::fractionAsPercent("0.025"), "2.50%");
    EXPECT_EQ(nf::fractionAsPercent("0"), "0.00%");
}

TEST(FormatShiftPointRight, ExactDecimalShift) {
    EXPECT_EQ(nf::shiftPointRight("0.05", 2), "5");
    EXPECT_EQ(nf::shiftPointRight("1.5", 1), "15");
    EXPECT_EQ(nf::shiftPointRight("0.125", 2), "12.5");
    EXPECT_EQ(nf::shiftPointRight("2", 2), "200");
}
