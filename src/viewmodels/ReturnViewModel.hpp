// viewmodels/ReturnViewModel.hpp — returns-panel presentation logic (returns shell build).
//
// The one piece of real returns logic in the shell. Holds NO line/money state: the engine
// owns the return context and re-emits a full ReturnContextLoaded on every edit, so QML binds
// the returnable lines directly to the bridge's ReturnLineModel and the totals to the
// ReturnCommitted event. Responsibilities: dispatch start/setLineQty/commit, gate the blind
// flag and the unsupported "both" refund mode (Phase A), derive completion from the engine's
// re-emitted selection, and fold the two-event lifecycle (ReturnCommitted → RefundSettled)
// into a display-only status. No QML dependency, so it is unit-tested headless against a
// bridge + ConfigService.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConfigService.hpp"

namespace blissmont::viewmodels {

class ReturnViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    Q_PROPERTY(blissmont::services::ConfigService* config READ config WRITE setConfig NOTIFY configChanged)
    // A return context is loaded and not yet committed (between ReturnContextLoaded and
    // ReturnCommitted). The panel shows the line selection while true.
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    // The original receipt the context was loaded from (panel title).
    Q_PROPERTY(QString originalReceiptNo READ originalReceiptNo NOTIFY stateChanged)
    // True when the return can be committed: a context is active, at least one line has a
    // positive selected qty, and the refund mode is supported (Phase A: not "both").
    Q_PROPERTY(bool canCommit READ canCommit NOTIFY stateChanged)
    // False when refund_tender_mode == "both" (split original+cash) — unsupported in Phase A.
    // The panel shows a "not supported on this terminal" banner and disables commit.
    Q_PROPERTY(bool refundModeSupported READ refundModeSupported NOTIFY stateChanged)
    // True when blind returns are permitted (allow_blind_return) — gates the blind affordance.
    Q_PROPERTY(bool allowBlind READ allowBlind NOTIFY stateChanged)
    // Free-text status: rejects, the provisional credit note, then its canonical reconcile.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ReturnViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] blissmont::services::ConfigService* config() const { return config_; }
    void setConfig(blissmont::services::ConfigService* config);

    // Derived from the bridge's ReturnLineModel: a context is active while it holds rows.
    // The bridge resets it on each ReturnContextLoaded and clears it on ReturnCommitted, so
    // this needs no separate flag (and stays drivable in tests via returnLines()->reset()).
    [[nodiscard]] bool active() const;
    [[nodiscard]] QString originalReceiptNo() const { return originalReceiptNo_; }
    [[nodiscard]] bool canCommit() const;
    [[nodiscard]] bool refundModeSupported() const;
    [[nodiscard]] bool allowBlind() const;
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

    // Load a finalized bill into a return context. Blocks (status, no command) when a blind
    // return is requested but allow_blind_return is off.
    Q_INVOKABLE void startReturn(const QString& receiptNo, bool blind);
    // Edit one line's selected qty / restock. The engine re-emits ReturnContextLoaded and the
    // ReturnLineModel resets — never mutate locally.
    Q_INVOKABLE void setLineQty(int originalLineNo, const QString& qty, bool restock);
    // Issue the credit note. Blocks (status, no command) when canCommit is false — including
    // the unsupported "both" refund mode (Phase A safety, mirrors the engine reject).
    Q_INVOKABLE void commit();

signals:
    void bridgeChanged();
    void configChanged();
    void stateChanged();
    void statusMessageChanged();

private:
    void setStatus(const QString& message);
    [[nodiscard]] bool hasSelectedLines() const;

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    blissmont::services::ConfigService* config_ = nullptr;
    QString originalReceiptNo_;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
