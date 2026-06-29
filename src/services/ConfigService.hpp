// services/ConfigService.hpp — resolved terminal config flags for VMs/QML (spec §5).
//
// Holds the server-resolved operating snapshot (which panels/actions are enabled, tender
// behaviour, return policy, …) so the UI can gate features without re-deriving the hierarchy.
//
// ── INTEGRATION GAP CLOSED (contracts v1.1.0) ─────────────────────────────────────────
// This was a validated UI gap (spec §3): the config snapshot (TerminalConfigSnapshot) lived
// only in blissmont.pos.v1 (the terminal<->SERVER contract), not in blissmont.terminal.v1 (the
// UI<->ENGINE contract the shell consumes), so the shell had no path to those flags.
//
// Closed exactly as anticipated — a CONTRACT change, not a patch here: contracts v1.1.0 adds a
// device-domain terminal.v1.TerminalConfig carried by a new ConfigUpdated arm of the existing
// Session Event oneof. The engine maps the server snapshot into that device-domain message and
// pushes it on connect, reconnect, and config change. ConfigService now HYDRATES from that event
// (applyConfig, wired to PosEngineBridge::configUpdated in QML), exactly like ConnectionService
// hydrates from SyncStatusChanged. The defaults below remain only as the pre-hydration fallback
// (before the first ConfigUpdated lands), so the skeleton still runs disconnected.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace blissmont::services {

class ConfigService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(bool allowReturns READ allowReturns NOTIFY changed)
    Q_PROPERTY(bool payoutEnabled READ payoutEnabled NOTIFY changed)
    Q_PROPERTY(bool allowDiscounts READ allowDiscounts NOTIFY changed)
    Q_PROPERTY(QString tenderCompleteMode READ tenderCompleteMode NOTIFY changed)
    Q_PROPERTY(QString currencySymbol READ currencySymbol NOTIFY changed)
    // Enabled tenders for the tender panel, sorted by sortOrder. Each entry is a
    // QVariantMap {method, displayName, hotkey, sortOrder, enabled, referenceMode}
    // (contracts v1.2.0). Disabled methods are dropped here — only tenderable
    // methods reach the UI.
    Q_PROPERTY(QVariantList enabledPaymentMethods READ enabledPaymentMethods NOTIFY changed)
    // Payout category KEYS for the payout panel's pick-list (contracts terminal.v1
    // TerminalConfig.payout_categories). Only the keys ride the device snapshot — the
    // category→GL-account mapping (pos.v1.PayoutCategory.gl_account) stays server-side.
    // A payout REQUIRES one of these, so the panel selects from this list.
    Q_PROPERTY(QStringList payoutCategories READ payoutCategories NOTIFY changed)
    // ── Returns policy (UX §9) — display/gate hints; the engine enforces each one ──
    Q_PROPERTY(bool allowBlindReturn READ allowBlindReturn NOTIFY changed)
    Q_PROPERTY(QString refundTenderMode READ refundTenderMode NOTIFY changed)     // original|cash|both
    Q_PROPERTY(QString returnRequiresAuth READ returnRequiresAuth NOTIFY changed) // always|config|never
    Q_PROPERTY(bool restockDefault READ restockDefault NOTIFY changed)
    Q_PROPERTY(bool allowPartialReturn READ allowPartialReturn NOTIFY changed)
    // ── Suspend/resume (UX §10) — display/UX only; the engine enforces expiry ──
    // Duration string (e.g. "24h"); empty means the engine's end-of-day default.
    Q_PROPERTY(QString heldCartExpiry READ heldCartExpiry NOTIFY changed)
    // ── Appearance (SHELL_KEYBOARD_LOOKUP brief, Part 2) — the configurable default theme.
    // "light" | "dark"; Theme.qml binds its initial mode to this. A live toggle in the UI
    // detaches from this, so a manual switch is not overridden by a config rehydrate.
    Q_PROPERTY(QString themeMode READ themeMode NOTIFY changed)

public:
    explicit ConfigService(QObject* parent = nullptr);

    [[nodiscard]] bool loaded() const { return loaded_; }
    [[nodiscard]] bool allowReturns() const { return allowReturns_; }
    [[nodiscard]] bool payoutEnabled() const { return payoutEnabled_; }
    [[nodiscard]] bool allowDiscounts() const { return allowDiscounts_; }
    [[nodiscard]] QString tenderCompleteMode() const { return tenderCompleteMode_; }
    [[nodiscard]] QString currencySymbol() const { return currencySymbol_; }
    [[nodiscard]] QVariantList enabledPaymentMethods() const { return enabledPaymentMethods_; }
    [[nodiscard]] QStringList payoutCategories() const { return payoutCategories_; }
    [[nodiscard]] bool allowBlindReturn() const { return allowBlindReturn_; }
    [[nodiscard]] QString refundTenderMode() const { return refundTenderMode_; }
    [[nodiscard]] QString returnRequiresAuth() const { return returnRequiresAuth_; }
    [[nodiscard]] bool restockDefault() const { return restockDefault_; }
    [[nodiscard]] bool allowPartialReturn() const { return allowPartialReturn_; }
    [[nodiscard]] QString heldCartExpiry() const { return heldCartExpiry_; }
    [[nodiscard]] QString themeMode() const { return themeMode_; }

public slots:
    // Hydrate from an engine ConfigUpdated event (relayed by PosEngineBridge, wired
    // in QML). Idempotent: re-applying the same values is a no-op; this is what makes
    // reconnect rehydration cheap (the engine re-pushes config on every (re)connect).
    // paymentMethods is the full device-domain list; this filters to enabled and
    // sorts by sortOrder before exposing it.
    void applyConfig(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                     const QString& tenderCompleteMode, const QString& currencySymbol,
                     const QVariantList& paymentMethods,
                     bool allowBlindReturn = false,
                     const QString& refundTenderMode = QStringLiteral("cash"),
                     const QString& returnRequiresAuth = QStringLiteral("never"),
                     bool restockDefault = false, bool allowPartialReturn = false,
                     const QString& heldCartExpiry = QString(),
                     const QStringList& payoutCategories = QStringList());

    // Appearance default (SHELL_KEYBOARD_LOOKUP brief, Part 2). Kept OFF the wired
    // ConfigUpdated arm (applyConfig stays arity-matched to the bridge signal — no contract
    // change). Theme.qml binds its initial mode to themeMode; this slot lets a future
    // appearance setting / settings UI change the configured default. Idempotent.
    void setThemeMode(const QString& mode);

signals:
    void changed();

private:
    // Safe defaults until the config-relay contract gap (above) is closed.
    bool loaded_ = false;
    bool allowReturns_ = true;
    bool payoutEnabled_ = true;
    bool allowDiscounts_ = true;
    QString tenderCompleteMode_ = QStringLiteral("confirm");
    QString currencySymbol_ = QStringLiteral("₹");  // ₹
    QVariantList enabledPaymentMethods_;            // enabled-only, sorted by sortOrder
    QStringList payoutCategories_;                  // payout category keys (display = key)
    bool allowBlindReturn_ = false;
    QString refundTenderMode_ = QStringLiteral("cash");
    QString returnRequiresAuth_ = QStringLiteral("never");
    bool restockDefault_ = false;
    bool allowPartialReturn_ = false;
    QString heldCartExpiry_;  // duration string e.g. "24h"; empty → engine end-of-day default
    QString themeMode_ = QStringLiteral("light");  // configurable default appearance; POS = light
};

}  // namespace blissmont::services
