// viewmodels/SuspendResumeViewModel.hpp — suspend/resume-panel presentation logic (UX §10).
//
// The thin piece of suspend/resume logic in the shell. Holds NO cart/draft state: the engine
// owns the parked drafts and re-emits a full CartUpdated (status="held") on resume, and the
// active holds bind directly to the bridge's HeldCartModel. Responsibilities: dispatch
// hold / listHeldCarts / resume, fold the engine's cartHeld confirmation + rejects into a
// display-only status, and notify when the holds list changes. No local cart mutation — held
// drafts are terminal-local and the engine is the single source of truth. No QML dependency,
// so it is unit-tested headless against a bridge.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"

namespace blissmont::viewmodels {

class SuspendResumeViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    // Free-text status: the hold confirmation ("Held: <label/id>") and engine rejects
    // (e.g. EMPTY_CART on a hold with nothing to park, NOT_FOUND on a stale resume).
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    // True once at least one active hold is listed — the panel shows the resume list while true.
    Q_PROPERTY(bool hasHolds READ hasHolds NOTIFY holdsChanged)

public:
    explicit SuspendResumeViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] QString statusMessage() const { return statusMessage_; }
    [[nodiscard]] bool hasHolds() const;

    // Park the current cart (UX §10 — instant, optional label). The engine mints the id and
    // echoes it via cartHeld; the cart then clears for the next customer. Blocked engine-side
    // (EMPTY_CART) when there is nothing to hold — surfaced as status, no local guard.
    Q_INVOKABLE void hold(const QString& label = QString());
    // Enter resume context: ask the engine for the active holds (fills the HeldCartModel).
    Q_INVOKABLE void openResume();
    // Resume a parked draft by id — the engine restores it and re-emits CartUpdated
    // (status="held"). Ignored for an empty id.
    Q_INVOKABLE void resume(const QString& heldCartId);

signals:
    void bridgeChanged();
    void statusMessageChanged();
    void holdsChanged();

private:
    void setStatus(const QString& message);

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
