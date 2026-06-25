// services/ConfigService.cpp — see ConfigService.hpp (note the documented integration gap).
#include "services/ConfigService.hpp"

namespace blissmont::services {

ConfigService::ConfigService(QObject* parent) : QObject(parent) {}

void ConfigService::applyConfig(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                                const QString& tenderCompleteMode, const QString& currencySymbol) {
    if (loaded_ && allowReturns_ == allowReturns && payoutEnabled_ == payoutEnabled &&
        allowDiscounts_ == allowDiscounts && tenderCompleteMode_ == tenderCompleteMode &&
        currencySymbol_ == currencySymbol) {
        return;  // unchanged — no spurious notify on every reconnect
    }
    loaded_ = true;
    allowReturns_ = allowReturns;
    payoutEnabled_ = payoutEnabled;
    allowDiscounts_ = allowDiscounts;
    tenderCompleteMode_ = tenderCompleteMode;
    currencySymbol_ = currencySymbol;
    emit changed();
}

}  // namespace blissmont::services
