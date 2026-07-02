// services/ConfigService.cpp — see ConfigService.hpp (note the documented integration gap).
#include "services/ConfigService.hpp"

#include <algorithm>

namespace blissmont::services {

namespace {

// Filter to enabled methods and order by sortOrder (stable on ties). The engine
// already orders, but the UI must not depend on arrival order.
QVariantList enabledSorted(const QVariantList& methods) {
    QVariantList out;
    out.reserve(methods.size());
    for (const QVariant& v : methods) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("enabled")).toBool()) out.push_back(row);
    }
    std::stable_sort(out.begin(), out.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value(QStringLiteral("sortOrder")).toInt() <
               b.toMap().value(QStringLiteral("sortOrder")).toInt();
    });
    return out;
}

// Canonical currency symbol (R1.3). The store config may carry the legacy ASCII
// abbreviation "Rs"/"Rs." for the SAME currency (INR); the running UI standard is the
// ₹ glyph. This normalizes that one glyph (it does NOT change the currency) and falls
// back to ₹ when the config sends nothing. A genuinely different currency symbol (e.g.
// "$") passes through unchanged.
QString canonicalSymbol(const QString& symbol) {
    const QString s = symbol.trimmed();
    if (s.isEmpty() || s.compare(QStringLiteral("Rs"), Qt::CaseInsensitive) == 0 ||
        s.compare(QStringLiteral("Rs."), Qt::CaseInsensitive) == 0 ||
        s.compare(QStringLiteral("INR"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("₹");
    }
    return s;
}

}  // namespace

ConfigService::ConfigService(QObject* parent) : QObject(parent) {}

void ConfigService::applyConfig(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                                const QString& tenderCompleteMode, const QString& currencySymbol,
                                const QVariantList& paymentMethods, bool allowBlindReturn,
                                const QString& refundTenderMode, const QString& returnRequiresAuth,
                                bool restockDefault, bool allowPartialReturn,
                                const QString& heldCartExpiry,
                                const QStringList& payoutCategories,
                                const QString& storeName, const QString& registerName) {
    const QVariantList methods = enabledSorted(paymentMethods);
    const QString symbol = canonicalSymbol(currencySymbol);
    if (loaded_ && allowReturns_ == allowReturns && payoutEnabled_ == payoutEnabled &&
        allowDiscounts_ == allowDiscounts && tenderCompleteMode_ == tenderCompleteMode &&
        currencySymbol_ == symbol && enabledPaymentMethods_ == methods &&
        payoutCategories_ == payoutCategories &&
        allowBlindReturn_ == allowBlindReturn && refundTenderMode_ == refundTenderMode &&
        returnRequiresAuth_ == returnRequiresAuth && restockDefault_ == restockDefault &&
        allowPartialReturn_ == allowPartialReturn && heldCartExpiry_ == heldCartExpiry &&
        storeName_ == storeName && registerName_ == registerName) {
        return;  // unchanged — no spurious notify on every reconnect
    }
    loaded_ = true;
    allowReturns_ = allowReturns;
    payoutEnabled_ = payoutEnabled;
    allowDiscounts_ = allowDiscounts;
    tenderCompleteMode_ = tenderCompleteMode;
    currencySymbol_ = symbol;
    enabledPaymentMethods_ = methods;
    payoutCategories_ = payoutCategories;
    allowBlindReturn_ = allowBlindReturn;
    refundTenderMode_ = refundTenderMode;
    returnRequiresAuth_ = returnRequiresAuth;
    restockDefault_ = restockDefault;
    allowPartialReturn_ = allowPartialReturn;
    heldCartExpiry_ = heldCartExpiry;
    storeName_ = storeName;
    registerName_ = registerName;
    emit changed();
}

void ConfigService::setThemeMode(const QString& mode) {
    if (themeMode_ == mode) return;  // idempotent — no churn on re-apply
    themeMode_ = mode;
    emit changed();
}

}  // namespace blissmont::services
