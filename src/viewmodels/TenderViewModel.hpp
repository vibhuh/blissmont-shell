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
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit TenderViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] blissmont::services::ConfigService* config() const { return config_; }
    void setConfig(blissmont::services::ConfigService* config);

    [[nodiscard]] bool canComplete() const;
    [[nodiscard]] bool autoComplete() const;
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

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

signals:
    void bridgeChanged();
    void configChanged();
    void stateChanged();
    void statusMessageChanged();

private:
    void setStatus(const QString& message);

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    blissmont::services::ConfigService* config_ = nullptr;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
