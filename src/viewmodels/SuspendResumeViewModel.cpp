// viewmodels/SuspendResumeViewModel.cpp — see SuspendResumeViewModel.hpp.
#include "viewmodels/SuspendResumeViewModel.hpp"

#include "models/HeldCartModel.hpp"

namespace blissmont::viewmodels {

SuspendResumeViewModel::SuspendResumeViewModel(QObject* parent) : QObject(parent) {}

void SuspendResumeViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    if (bridge_) {
        using B = blissmont::bridge::PosEngineBridge;
        // The holds model resets on every HeldCartsList — re-drive hasHolds off it (and stay
        // drivable in tests via heldCarts()->reset()).
        if (bridge_->heldCarts()) {
            connect(bridge_->heldCarts(), &QAbstractItemModel::modelReset, this,
                    &SuspendResumeViewModel::holdsChanged);
        }
        // The engine echoes the minted id after a hold — surface the confirmation. Prefer the
        // cashier's label; fall back to a short id so the slip is always identifiable.
        connect(bridge_, &B::cartHeld, this, [this](const QString& heldCartId, const QString& label) {
            const QString tag = label.isEmpty() ? QStringLiteral("#") + heldCartId.left(8) : label;
            setStatus(tr("Held: %1").arg(tag));
        });
        // Engine rejects (e.g. EMPTY_CART, NOT_FOUND on a stale resume) surface verbatim.
        connect(bridge_, &B::commandRejected, this,
                [this](const QString& code, const QString& message) {
                    setStatus(message.isEmpty() ? code : message);
                });
    }
    emit bridgeChanged();
    emit holdsChanged();
}

bool SuspendResumeViewModel::hasHolds() const {
    return bridge_ && bridge_->heldCarts() && bridge_->heldCarts()->rowCount() > 0;
}

void SuspendResumeViewModel::hold(const QString& label) {
    if (!bridge_) return;
    bridge_->holdCart(label);
    setStatus(QString());  // the cartHeld confirmation (or a reject) replaces this
}

void SuspendResumeViewModel::openResume() {
    if (!bridge_) return;
    setStatus(QString());
    bridge_->listHeldCarts();
}

void SuspendResumeViewModel::resume(const QString& heldCartId) {
    if (!bridge_ || heldCartId.trimmed().isEmpty()) return;
    setStatus(QString());
    bridge_->resumeCart(heldCartId.trimmed());
}

void SuspendResumeViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
