// core/Format.hpp — the ONE presentation-formatting standard for numbers (refinement
// brief, Phase 1). The engine is the source of truth for all money/quantity math and
// emits amounts as full-precision decimal strings ("1003.0000000000", "1.0000000000").
// Raw precision must NEVER reach the cashier's eye — every numeric value rendered in the
// UI is routed through these helpers so columns line up and digits don't jitter.
//
// Qt-free on purpose: this is core/, unit-tested with GoogleTest and no display. The QML
// layer (services/FormatService — the `Format` singleton) wraps these and adds the
// currency symbol; the C++ view-models call them directly for status strings.
//
// Rounding is half-up, done on the decimal STRING (no double, no int64 overflow on long
// integer parts), so it stays bit-faithful to the engine's decimal model.
#pragma once

#include <string>
#include <string_view>

namespace blissmont::core::numfmt {

// Round a decimal string half-up to exactly `decimals` fraction digits. Ungrouped, ASCII
// sign, leading zeros stripped, no "-0.00". Junk/empty -> zero. Always emits exactly
// `decimals` fraction digits (none when decimals == 0).
//   round("1003.0000000000", 2) -> "1003.00";  round("40.5", 2) -> "40.50"
//   round("1.235", 2) -> "1.24" (half-up);      round("", 2) -> "0.00"
std::string round(std::string_view decimal, int decimals);

// Group the integer part of an already-normalized number with the Indian system
// (1,00,000 — lakh/crore), preserving sign and fraction. group("1003.00") -> "1,003.00";
// group("100000.00") -> "1,00,000.00". This is an INR (₹/GST) POS, so Indian grouping is
// the locally-correct convention.
std::string group(std::string_view number);

// Money: grouped, exactly 2 decimals, NO currency symbol (the symbol is applied by the
// QML `Format` singleton). money("1003.0000000000") -> "1,003.00"; money("") -> "0.00".
std::string money(std::string_view decimal);

// Quantity: at most `precision` fraction digits, trailing zeros trimmed; whole numbers
// render bare. qty("1.0000000000") -> "1"; qty("1.2500000000") -> "1.25";
// qty("0.750") -> "0.75". Never "1.0000000000".
std::string qty(std::string_view decimal, int precision = 3);

// Percent: fixed 2 decimals + '%'. The input is the percentage value already.
// percent("18") -> "18.00%"; percent("5") -> "5.00%".
std::string percent(std::string_view decimal);

// A fraction rendered as a percent (the engine's tax_rate_str is a fraction).
// fractionAsPercent("0.05") -> "5.00%"; fractionAsPercent("0.18") -> "18.00%".
std::string fractionAsPercent(std::string_view fraction);

// Multiply a decimal string by 10^k by shifting the decimal point right (exact, no float).
// shiftPointRight("0.05", 2) -> "5"; shiftPointRight("1.5", 1) -> "15".
std::string shiftPointRight(std::string_view decimal, int k);

}  // namespace blissmont::core::numfmt
