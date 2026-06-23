// services/ConfigService.hpp — resolved terminal config flags for VMs/QML (spec §5).
//
// Holds the server-resolved operating snapshot (which panels/actions are enabled, tender
// behaviour, return policy, …) so the UI can gate features without re-deriving the hierarchy.
//
// ── KNOWN INTEGRATION GAP (a validated UI gap, spec §3) ───────────────────────────────
// The config snapshot (TerminalConfigSnapshot) is defined in blissmont.pos.v1 (pos.proto,
// the terminal<->SERVER contract), NOT in blissmont.terminal.v1 (terminal.proto, the
// UI<->ENGINE contract the shell consumes). The shell talks only to the local engine, so it
// has no path to those flags yet. Closing this gap is a CONTRACT change in blissmont-contracts
// (preferred: the engine relays the resolved config to the UI over the Session stream, e.g. a
// new ConfigSnapshot event in terminal.proto), then a submodule bump + regen — never a patch
// here. Until then ConfigService serves safe defaults so the skeleton runs.
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
