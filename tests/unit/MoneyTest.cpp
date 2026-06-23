// tests/unit/MoneyTest.cpp — exact decimal-string <-> minor-unit conversion (core/Money).
// Pure logic, no Qt. This is the kind of engine-mirroring rule that must never drift.
#include <gtest/gtest.h>

#include "core/Money.hpp"

using blissmont::core::Money;

TEST(MoneyParse, IntegerAndFraction) {
    EXPECT_EQ(Money::parse("1234.50").value().minorUnits(), 123450);
    EXPECT_EQ(Money::parse("12").value().minorUnits(), 1200);
    EXPECT_EQ(Money::parse("0.07").value().minorUnits(), 7);
    EXPECT_EQ(Money::parse("1.5").value().minorUnits(), 150);  // one fraction digit -> paise
    EXPECT_EQ(Money::parse("-5").value().minorUnits(), -500);
    EXPECT_EQ(Money::parse("0").value().minorUnits(), 0);
}

TEST(MoneyParse, RejectsJunk) {
    EXPECT_FALSE(Money::parse(""));
    EXPECT_FALSE(Money::parse("abc"));
    EXPECT_FALSE(Money::parse("1.234"));   // >2 fraction digits
    EXPECT_FALSE(Money::parse("1."));      // trailing point
    EXPECT_FALSE(Money::parse("."));       // no integer part
    EXPECT_FALSE(Money::parse("1.2.3"));   // second non-digit in fraction
}

TEST(MoneyFormat, FixedTwoDecimals) {
    EXPECT_EQ(Money(123450).toString(), "1234.50");
    EXPECT_EQ(Money(7).toString(), "0.07");
    EXPECT_EQ(Money(0).toString(), "0.00");
    EXPECT_EQ(Money(-500).toString(), "-5.00");
}

TEST(MoneyRoundTrip, ParseFormatStable) {
    for (const char* s : {"0.00", "1234.50", "0.07", "999999.99", "-12.34"}) {
        const auto m = Money::parse(s);
        ASSERT_TRUE(m) << s;
        EXPECT_EQ(m.value().toString(), s) << s;
    }
}

TEST(MoneyArithmetic, Exact) {
    const auto a = Money::parse("10.10").value();
    const auto b = Money::parse("0.20").value();
    EXPECT_EQ((a + b).toString(), "10.30");
    EXPECT_EQ((a - b).toString(), "9.90");
    EXPECT_TRUE(b < a);
}
