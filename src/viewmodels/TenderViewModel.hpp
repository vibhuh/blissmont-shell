// viewmodels/TenderViewModel.hpp — tender-panel presentation logic (tender shell build).
//
// The one piece of real tender logic in the shell. Holds NO money/cart state (the engine
// owns it; QML binds tenders/totals directly to the bridge's TenderListModel + CartSummary).
// Responsibilities: gate addTender on the per-method reference policy (reference_mode),
// dispatch removeTender, and derive completion (settle) from the engine's balance — never
// recompute money beyond a sign check on the engine's exact decimal string. No QML
// dependency, so it is unit-tested headless against a bridge + ConfigService.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConfigService.hpp"

namespace blissmont::viewmodels {

class TenderViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    Q_PROPERTY(blissmont::services::ConfigService* config READ config WRITE setConfig NOTIFY configChanged)
    // True when the bill can be completed: the engine's balance due is <= 0.
    Q_PROPERTY(bool canComplete READ canComplete NOTIFY stateChanged)
    // True when the store completes tenders automatically on balance-zero
    // (tenderCompleteMode == "auto"); the panel uses it to decide whether to settle
    // without an explicit confirm. Policy lives in config, not hardcoded here.
    Q_PROPERTY(bool autoComplete READ autoComplete NOTIFY stateChanged)
    // The primary (cash) payment path the calculator opens on: the enabled method named
    // "cash", else the first enabled method. Empty only when no methods are configured.
    Q_PROPERTY(QString primaryMethod READ primaryMethod NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit TenderViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] blissmont::services::ConfigService* config() const { return config_; }
    void setConfig(blissmont::services::ConfigService* config);

    [[nodiscard]] bool canComplete() const;
    [[nodiscard]] bool autoComplete() const;
    [[nodiscard]] QString primaryMethod() const;
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

    // Live CHANGE preview as the cashier types (Change = Received − bill Total). DISPLAY
    // only — the engine computes the authoritative change at settle; this never touches a
    // double (exact paise via core::Money) and is clamped at 0 (no negative "change" when
    // short). Both inputs are rounded to 2 dp first so the engine's full-precision total
    // string parses. Returns a bare decimal ("47.00"); the caller formats it.
    Q_INVOKABLE QString changeDuePreview(const QString& received) const;

    // The per-method reference policy from config ("none" | "optional" | "required");
    // "none" if the method is unknown. QML uses it to show/require the reference field.
    Q_INVOKABLE QString referenceModeFor(const QString& method) const;

    // Add a tender. Blocks (with a status message, no command sent) when the method's
    // reference_mode is "required" and the reference is blank. Otherwise dispatches to
    // the engine, which recomputes the balance and pushes a fresh snapshot.
    Q_INVOKABLE void addTender(const QString& method, const QString& amount,
                               const QString& reference);
    Q_INVOKABLE void removeTender(int tenderNo);
    // Settle the bill. No-op (with a status message) unless canComplete.
    Q_INVOKABLE void complete();

    // Calculator path (refinement brief, Phase 2): tender `amount` via `method`, then
    // settle as soon as the engine reports the balance cleared — the keyboard-first
    // "Enter completes the sale". Reference gating matches addTender (a required-but-blank
    // reference blocks, with a status message, and does NOT arm completion). Underpayment
    // just records the tender and waits (split): completion fires when the balance clears.
    Q_INVOKABLE void tenderAndComplete(const QString& method, const QString& amount,
                                       const QString& reference);

signals:
    void bridgeChanged();
    void configChanged();
    void stateChanged();
    void statusMessageChanged();

private:
    void setStatus(const QString& message);
    // Re-drive completion gating on each engine snapshot; fire the armed settle when the
    // balance has cleared.
    void onBalanceChanged();

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    blissmont::services::ConfigService* config_ = nullptr;
    QString statusMessage_;
    // Armed by tenderAndComplete; settles on the next snapshot where canComplete is true.
    bool completeArmed_ = false;
};

}  // namespace blissmont::viewmodels
