// core/Money.hpp — exact money value type for the shell's display layer.
//
// The engine is the single source of truth for all monetary math and emits amounts as
// decimal strings (e.g. "1234.50"); see blissmont-contracts terminal.proto (*_str fields).
// The shell never does pricing/tax math — it only PARSES those strings to render them and
// FORMATS user-entered amounts to send back. To stay bit-exact with the engine's decimal
// model we hold an integer count of minor units (paise) internally and never touch double.
//
// Qt-free on purpose: this is core/, unit-tested with GoogleTest and no display.
#pragma once

#include <cstdint>
#include <string>

#include "core/Result.hpp"

namespace blissmont::core {

class Money {
public:
    static constexpr int kMinorUnitsPerMajor = 100;  // INR paise; 2 fraction digits

    Money() = default;
    explicit Money(std::int64_t minorUnits) : minor_(minorUnits) {}

    // Parse a decimal string as emitted by the engine ("1234.50", "-5", "0.07", "12").
    // Rejects junk, >2 fraction digits, and empty input. Never throws.
    static Result<Money> parse(const std::string& decimal);

    // Format back to a fixed-2-dp decimal string ("1234.50"), round-trippable with parse().
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::int64_t minorUnits() const noexcept { return minor_; }
    [[nodiscard]] bool isZero() const noexcept { return minor_ == 0; }

    Money operator+(const Money& o) const { return Money(minor_ + o.minor_); }
    Money operator-(const Money& o) const { return Money(minor_ - o.minor_); }
    bool operator==(const Money& o) const noexcept { return minor_ == o.minor_; }
    auto operator<=>(const Money& o) const noexcept { return minor_ <=> o.minor_; }

private:
    std::int64_t minor_ = 0;
};

}  // namespace blissmont::core
