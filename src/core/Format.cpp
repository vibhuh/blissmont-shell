// core/Format.cpp — see Format.hpp. Exact, string-based decimal presentation formatting.
#include "core/Format.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace blissmont::core::numfmt {
namespace {

// Add 1 to a non-negative decimal digit string, growing it on carry ("999" -> "1000").
std::string incrementDigits(std::string d) {
    int i = static_cast<int>(d.size()) - 1;
    for (; i >= 0; --i) {
        if (d[i] == '9') {
            d[i] = '0';
        } else {
            ++d[i];
            break;
        }
    }
    if (i < 0) d.insert(d.begin(), '1');
    return d;
}

std::string zeroWith(int decimals) {
    std::string z = "0";
    if (decimals > 0) {
        z.push_back('.');
        z.append(static_cast<std::size_t>(decimals), '0');
    }
    return z;
}

}  // namespace

std::string round(std::string_view s, int decimals) {
    if (decimals < 0) decimals = 0;

    // Trim surrounding whitespace.
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    s = s.substr(b, e - b);
    if (s.empty()) return zeroWith(decimals);

    std::size_t i = 0;
    bool negative = false;
    if (s[i] == '+' || s[i] == '-') {
        negative = (s[i] == '-');
        ++i;
    }

    std::string intPart, fracPart;
    bool seenDot = false, anyDigit = false;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '.') {
            if (seenDot) return zeroWith(decimals);  // two dots -> junk
            seenDot = true;
            continue;
        }
        if (c < '0' || c > '9') return zeroWith(decimals);  // junk
        anyDigit = true;
        if (seenDot) {
            fracPart.push_back(c);
        } else {
            intPart.push_back(c);
        }
    }
    if (!anyDigit) return zeroWith(decimals);

    // Half-up: round up when the first dropped fraction digit is >= 5.
    const bool roundUp = (static_cast<int>(fracPart.size()) > decimals) &&
                         (fracPart[static_cast<std::size_t>(decimals)] >= '5');

    // Combined integer of value * 10^decimals: intPart followed by the kept fraction
    // digits (truncated, then zero-padded up to `decimals`).
    std::string frac = fracPart.substr(
        0, std::min(static_cast<std::size_t>(decimals), fracPart.size()));
    while (static_cast<int>(frac.size()) < decimals) frac.push_back('0');
    std::string digits = intPart + frac;
    if (digits.empty()) digits = "0";
    if (roundUp) digits = incrementDigits(digits);

    // Split the combined integer back into integer / fraction parts.
    std::string outInt, outFrac;
    if (decimals == 0) {
        outInt = digits;
    } else if (static_cast<int>(digits.size()) <= decimals) {
        outInt = "0";
        outFrac = std::string(static_cast<std::size_t>(decimals) - digits.size(), '0') + digits;
    } else {
        outInt = digits.substr(0, digits.size() - static_cast<std::size_t>(decimals));
        outFrac = digits.substr(digits.size() - static_cast<std::size_t>(decimals));
    }

    // Strip leading zeros in the integer part (keep one).
    std::size_t z = 0;
    while (z + 1 < outInt.size() && outInt[z] == '0') ++z;
    outInt = outInt.substr(z);

    const bool allZero =
        outInt == "0" && outFrac.find_first_not_of('0') == std::string::npos;

    std::string out;
    if (negative && !allZero) out.push_back('-');
    out += outInt;
    if (decimals > 0) {
        out.push_back('.');
        out += outFrac;
    }
    return out;
}

std::string group(std::string_view s) {
    std::string str(s);
    std::string sign;
    std::size_t i = 0;
    if (!str.empty() && (str[0] == '-' || str[0] == '+')) {
        if (str[0] == '-') sign = "-";
        i = 1;
    }
    const std::string rest = str.substr(i);
    const std::size_t dot = rest.find('.');
    const std::string ip = dot == std::string::npos ? rest : rest.substr(0, dot);
    const std::string fp = dot == std::string::npos ? std::string() : rest.substr(dot);  // incl '.'

    const int n = static_cast<int>(ip.size());
    std::string grouped;
    if (n <= 3) {
        grouped = ip;
    } else {
        // Last three digits stay together; the rest is grouped in twos (Indian system).
        const std::string head = ip.substr(0, static_cast<std::size_t>(n) - 3);
        const std::string tail = ip.substr(static_cast<std::size_t>(n) - 3);
        std::vector<std::string> groups;
        int idx = static_cast<int>(head.size());
        while (idx > 2) {
            groups.push_back(head.substr(static_cast<std::size_t>(idx) - 2, 2));
            idx -= 2;
        }
        if (idx > 0) groups.push_back(head.substr(0, static_cast<std::size_t>(idx)));
        std::reverse(groups.begin(), groups.end());
        for (const auto& g : groups) {
            grouped += g;
            grouped.push_back(',');
        }
        grouped += tail;
    }
    return sign + grouped + fp;
}

std::string money(std::string_view decimal) { return group(round(decimal, 2)); }

std::string qty(std::string_view decimal, int precision) {
    std::string r = round(decimal, precision);  // e.g. "1.250", "1.000", "12.235"
    const std::size_t dot = r.find('.');
    if (dot == std::string::npos) return r;
    std::size_t last = r.find_last_not_of('0');
    if (r[last] == '.') --last;  // whole number -> drop the dot too
    return r.substr(0, last + 1);
}

std::string percent(std::string_view decimal) { return round(decimal, 2) + "%"; }

std::string shiftPointRight(std::string_view s, int k) {
    if (k <= 0) return std::string(s);
    std::string str(s);
    std::string sign;
    std::size_t i = 0;
    if (!str.empty() && (str[0] == '-' || str[0] == '+')) {
        if (str[0] == '-') sign = "-";
        i = 1;
    }
    const std::string rest = str.substr(i);
    const std::size_t dot = rest.find('.');
    std::string ip = dot == std::string::npos ? rest : rest.substr(0, dot);
    std::string fp = dot == std::string::npos ? std::string() : rest.substr(dot + 1);

    while (static_cast<int>(fp.size()) < k) fp.push_back('0');
    ip += fp.substr(0, static_cast<std::size_t>(k));
    fp = fp.substr(static_cast<std::size_t>(k));

    // Strip leading zeros in the integer part (keep one).
    std::size_t z = 0;
    while (z + 1 < ip.size() && ip[z] == '0') ++z;
    ip = ip.substr(z);

    std::string out = sign + ip;
    if (!fp.empty()) out += "." + fp;
    return out;
}

std::string fractionAsPercent(std::string_view fraction) {
    return percent(shiftPointRight(fraction, 2));
}

}  // namespace blissmont::core::numfmt
