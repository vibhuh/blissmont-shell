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

public:
    explicit ConfigService(QObject* parent = nullptr);

    [[nodiscard]] bool loaded() const { return loaded_; }
    [[nodiscard]] bool allowReturns() const { return allowReturns_; }
    [[nodiscard]] bool payoutEnabled() const { return payoutEnabled_; }
    [[nodiscard]] bool allowDiscounts() const { return allowDiscounts_; }
    [[nodiscard]] QString tenderCompleteMode() const { return tenderCompleteMode_; }
    [[nodiscard]] QString currencySymbol() const { return currencySymbol_; }

public slots:
    // Hydrate from an engine ConfigUpdated event (relayed by PosEngineBridge, wired
    // in QML). Idempotent: re-applying the same values is a no-op; this is what makes
    // reconnect rehydration cheap (the engine re-pushes config on every (re)connect).
    void applyConfig(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                     const QString& tenderCompleteMode, const QString& currencySymbol);

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
};

}  // namespace blissmont::services
