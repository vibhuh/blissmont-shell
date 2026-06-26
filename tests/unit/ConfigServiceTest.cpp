// tests/unit/ConfigServiceTest.cpp — ConfigService hydration, headless, no engine.
// Proves the v1.1.0 gap closure at the service boundary: a ConfigUpdated payload
// (delivered here directly via applyConfig, exactly as PosEngineBridge::configUpdated
// would) populates the view-facing properties and flips `loaded`. The reconnect
// re-apply is idempotent — re-pushing the same config on every (re)connect must not
// emit a spurious `changed()`.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QVariantList>
#include <QVariantMap>

#include "services/ConfigService.hpp"

using blissmont::services::ConfigService;

namespace {
// Build one device-domain payment-method row as the bridge projects it.
QVariantMap pm(const QString& method, int sortOrder, bool enabled,
               const QString& referenceMode = QStringLiteral("none"),
               const QString& hotkey = QString()) {
    return QVariantMap{
        {QStringLiteral("method"), method},
        {QStringLiteral("displayName"), method.toUpper()},
        {QStringLiteral("hotkey"), hotkey},
        {QStringLiteral("sortOrder"), sortOrder},
        {QStringLiteral("enabled"), enabled},
        {QStringLiteral("referenceMode"), referenceMode},
    };
}
}  // namespace

TEST(ConfigService, DefaultsUnloadedUntilFirstConfig) {
    ConfigService cfg;
    EXPECT_FALSE(cfg.loaded());  // pre-hydration fallback only
}

TEST(ConfigService, ApplyConfigHydratesPropertiesAndFlipsLoaded) {
    ConfigService cfg;
    QSignalSpy spy(&cfg, &ConfigService::changed);

    cfg.applyConfig(/*allowReturns=*/false, /*payoutEnabled=*/false,
                    /*allowDiscounts=*/true, QStringLiteral("auto"), QStringLiteral("Rs"), {});

    EXPECT_TRUE(cfg.loaded());
    EXPECT_FALSE(cfg.allowReturns());
    EXPECT_FALSE(cfg.payoutEnabled());
    EXPECT_TRUE(cfg.allowDiscounts());
    EXPECT_EQ(cfg.tenderCompleteMode(), QStringLiteral("auto"));
    EXPECT_EQ(cfg.currencySymbol(), QStringLiteral("Rs"));
    EXPECT_EQ(spy.count(), 1);
}

TEST(ConfigService, ReapplyingSameConfigIsIdempotent) {
    ConfigService cfg;
    QSignalSpy spy(&cfg, &ConfigService::changed);

    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("₹"), {});
    // The engine re-pushes config on every (re)connect; an unchanged payload must
    // not churn bindings.
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("₹"), {});

    EXPECT_EQ(spy.count(), 1);  // exactly one notify across two identical applies
    EXPECT_TRUE(cfg.loaded());
}

TEST(ConfigService, RehydrationAppliesChangedConfig) {
    ConfigService cfg;
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("₹"), {});

    QSignalSpy spy(&cfg, &ConfigService::changed);
    // Reconnect after a server-side config change: new values must take effect.
    cfg.applyConfig(false, false, true, QStringLiteral("auto"), QStringLiteral("Rs"), {});

    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(cfg.allowReturns());
    EXPECT_EQ(cfg.tenderCompleteMode(), QStringLiteral("auto"));
}

// ── Payment methods (contracts v1.2.0) ────────────────────────────────────────

TEST(ConfigService, EnabledPaymentMethodsDropsDisabledAndSortsBySortOrder) {
    ConfigService cfg;
    QVariantList methods{
        pm(QStringLiteral("card"), /*sortOrder=*/2, /*enabled=*/true),
        pm(QStringLiteral("cash"), /*sortOrder=*/0, /*enabled=*/true),
        pm(QStringLiteral("voucher"), /*sortOrder=*/1, /*enabled=*/false),  // dropped
    };
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"), methods);

    const QVariantList e = cfg.enabledPaymentMethods();
    ASSERT_EQ(e.size(), 2);  // voucher (disabled) dropped
    EXPECT_EQ(e[0].toMap().value("method").toString(), QStringLiteral("cash"));  // sortOrder 0
    EXPECT_EQ(e[1].toMap().value("method").toString(), QStringLiteral("card"));  // sortOrder 2
}

TEST(ConfigService, ReapplyingSamePaymentMethodsIsIdempotent) {
    ConfigService cfg;
    QVariantList methods{pm(QStringLiteral("cash"), 0, true),
                         pm(QStringLiteral("upi"), 1, true, QStringLiteral("required"))};
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"), methods);

    QSignalSpy spy(&cfg, &ConfigService::changed);
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"), methods);
    EXPECT_EQ(spy.count(), 0);  // unchanged methods -> no churn

    // A change in the methods alone must notify.
    QVariantList changed{pm(QStringLiteral("cash"), 0, true)};
    cfg.applyConfig(true, true, true, QStringLiteral("confirm"), QStringLiteral("Rs"), changed);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(cfg.enabledPaymentMethods().size(), 1);
}
