// viewmodels/PayoutViewModel.hpp — payout-panel presentation logic (UX §12).
//
// The thin piece of payout logic in the shell. Holds NO money/GL state: a payout is a
// money-movement the engine + server own (the server posts the balanced journal by category
// — see rachis-core ADR-0008/0009); this VM only collects the cashier's input (amount,
// category, optional note), dispatches RecordPayout, and folds the engine's PayoutRecorded
// confirmation + rejects into a display-only status. The category list itself comes from
// ConfigService.payoutCategories (the server-resolved keys); the panel selects one and the
// engine resolves it to a GL account — that mapping never reaches the device. No QML
// dependency, so it is unit-tested headless against a bridge.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"

namespace blissmont::viewmodels {

class PayoutViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    // Free-text status: the payout confirmation ("Paid out: <amount> (<category>)") and engine
    // rejects (e.g. an unknown/disabled category, no open period) surfaced verbatim.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit PayoutViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

    // Dispatch a payout (UX §12). A category is REQUIRED (the server rejects a payout without
    // one); the amount must be non-empty — both are light presentation gates, the engine +
    // server enforce the real rules (positive amount, known category, open period). On success
    // the engine echoes PayoutRecorded → `recorded` fires so the panel clears.
    Q_INVOKABLE void recordPayout(const QString& amount, const QString& category,
                                  const QString& note = QString());

signals:
    void bridgeChanged();
    void statusMessageChanged();
    // The engine confirmed the payout — the panel clears its form on this. Carries the
    // accepted amount + category for the confirmation, never to re-key anything.
    void recorded(const QString& amount, const QString& category);

private:
    void setStatus(const QString& message);

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
