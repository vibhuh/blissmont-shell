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

}  // namespace

ConfigService::ConfigService(QObject* parent) : QObject(parent) {}

void ConfigService::applyConfig(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                                const QString& tenderCompleteMode, const QString& currencySymbol,
                                const QVariantList& paymentMethods, bool allowBlindReturn,
                                const QString& refundTenderMode, const QString& returnRequiresAuth,
                                bool restockDefault, bool allowPartialReturn,
                                const QString& heldCartExpiry) {
    const QVariantList methods = enabledSorted(paymentMethods);
    if (loaded_ && allowReturns_ == allowReturns && payoutEnabled_ == payoutEnabled &&
        allowDiscounts_ == allowDiscounts && tenderCompleteMode_ == tenderCompleteMode &&
        currencySymbol_ == currencySymbol && enabledPaymentMethods_ == methods &&
        allowBlindReturn_ == allowBlindReturn && refundTenderMode_ == refundTenderMode &&
        returnRequiresAuth_ == returnRequiresAuth && restockDefault_ == restockDefault &&
        allowPartialReturn_ == allowPartialReturn && heldCartExpiry_ == heldCartExpiry) {
        return;  // unchanged — no spurious notify on every reconnect
    }
    loaded_ = true;
    allowReturns_ = allowReturns;
    payoutEnabled_ = payoutEnabled;
    allowDiscounts_ = allowDiscounts;
    tenderCompleteMode_ = tenderCompleteMode;
    currencySymbol_ = currencySymbol;
    enabledPaymentMethods_ = methods;
    allowBlindReturn_ = allowBlindReturn;
    refundTenderMode_ = refundTenderMode;
    returnRequiresAuth_ = returnRequiresAuth;
    restockDefault_ = restockDefault;
    allowPartialReturn_ = allowPartialReturn;
    heldCartExpiry_ = heldCartExpiry;
    emit changed();
}

}  // namespace blissmont::services
