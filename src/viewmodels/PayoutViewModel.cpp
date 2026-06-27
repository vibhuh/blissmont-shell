// viewmodels/PayoutViewModel.cpp — see PayoutViewModel.hpp.
#include "viewmodels/PayoutViewModel.hpp"

namespace blissmont::viewmodels {

PayoutViewModel::PayoutViewModel(QObject* parent) : QObject(parent) {}

void PayoutViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    if (bridge_) {
        using B = blissmont::bridge::PosEngineBridge;
        // The engine echoes the recorded payout — surface the confirmation and let the panel
        // clear its form via `recorded`.
        connect(bridge_, &B::payoutRecorded, this,
                [this](const QString& /*payoutId*/, const QString& amount, const QString& category) {
                    setStatus(tr("Paid out: %1 (%2)").arg(amount, category));
                    emit recorded(amount, category);
                });
        // Engine rejects (unknown/disabled category, no open accounting period, …) surface
        // verbatim — the same display-only treatment as the other panels.
        connect(bridge_, &B::commandRejected, this,
                [this](const QString& code, const QString& message) {
                    setStatus(message.isEmpty() ? code : message);
                });
    }
    emit bridgeChanged();
}

void PayoutViewModel::recordPayout(const QString& amount, const QString& category,
                                   const QString& note) {
    if (!bridge_) return;
    // Light presentation gates; the engine + server enforce the real rules.
    if (amount.trimmed().isEmpty() || category.trimmed().isEmpty()) {
        setStatus(tr("Enter an amount and choose a category."));
        return;
    }
    setStatus(QString());  // the PayoutRecorded confirmation (or a reject) replaces this
    bridge_->recordPayout(amount.trimmed(), category, note.trimmed());
}

void PayoutViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
