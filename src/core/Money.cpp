// core/Money.cpp — see Money.hpp. Exact decimal-string <-> minor-unit conversion.
#include "core/Money.hpp"

#include <cstdlib>

namespace blissmont::core {

Result<Money> Money::parse(const std::string& decimal) {
    if (decimal.empty()) {
        return Result<Money>::Err("money.parse", "empty amount");
    }

    std::size_t i = 0;
    bool negative = false;
    if (decimal[i] == '+' || decimal[i] == '-') {
        negative = (decimal[i] == '-');
        ++i;
    }
    if (i >= decimal.size()) {
        return Result<Money>::Err("money.parse", "no digits in amount");
    }

    std::int64_t major = 0;
    std::size_t majorDigits = 0;
    for (; i < decimal.size() && decimal[i] != '.'; ++i) {
        const char c = decimal[i];
        if (c < '0' || c > '9') {
            return Result<Money>::Err("money.parse", "non-digit in integer part: " + decimal);
        }
        major = major * 10 + (c - '0');
        ++majorDigits;
    }
    if (majorDigits == 0) {
        return Result<Money>::Err("money.parse", "missing integer part: " + decimal);
    }

    std::int64_t minor = 0;
    if (i < decimal.size() && decimal[i] == '.') {
        ++i;  // consume '.'
        std::size_t fracDigits = 0;
        for (; i < decimal.size(); ++i, ++fracDigits) {
            const char c = decimal[i];
            if (c < '0' || c > '9') {
                return Result<Money>::Err("money.parse", "non-digit in fraction: " + decimal);
            }
            if (fracDigits >= 2) {
                return Result<Money>::Err("money.parse", "more than 2 fraction digits: " + decimal);
            }
            minor = minor * 10 + (c - '0');
        }
        if (fracDigits == 1) minor *= 10;  // "1.5" -> 50 paise
        if (fracDigits == 0) {
            return Result<Money>::Err("money.parse", "trailing decimal point: " + decimal);
        }
    }

    std::int64_t total = major * Money::kMinorUnitsPerMajor + minor;
    if (negative) total = -total;
    return Result<Money>::Ok(Money(total));
}

std::string Money::toString() const {
    const bool negative = minor_ < 0;
    std::int64_t abs = negative ? -minor_ : minor_;
    const std::int64_t major = abs / Money::kMinorUnitsPerMajor;
    const std::int64_t minor = abs % Money::kMinorUnitsPerMajor;

    std::string out;
    if (negative) out.push_back('-');
    out += std::to_string(major);
    out.push_back('.');
    out.push_back(static_cast<char>('0' + (minor / 10)));
    out.push_back(static_cast<char>('0' + (minor % 10)));
    return out;
}

}  // namespace blissmont::core
