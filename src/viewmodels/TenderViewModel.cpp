// viewmodels/TenderViewModel.cpp — see TenderViewModel.hpp.
#include "viewmodels/TenderViewModel.hpp"

#include "core/Money.hpp"

namespace blissmont::viewmodels {

TenderViewModel::TenderViewModel(QObject* parent) : QObject(parent) {}

void TenderViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    // The balance is derived from the engine's CartSummary; re-evaluate completion
    // whenever a fresh snapshot lands.
    if (bridge_ && bridge_->summary()) {
        connect(bridge_->summary(), &blissmont::models::CartSummary::changed, this,
                &TenderViewModel::stateChanged);
    }
    emit bridgeChanged();
    emit stateChanged();
}

void TenderViewModel::setConfig(blissmont::services::ConfigService* config) {
    if (config_ == config) return;
    config_ = config;
    if (config_) {
        // tenderCompleteMode / methods can change on a config push.
        connect(config_, &blissmont::services::ConfigService::changed, this,
                &TenderViewModel::stateChanged);
    }
    emit configChanged();
    emit stateChanged();
}

QString TenderViewModel::referenceModeFor(const QString& method) const {
    if (!config_) return QStringLiteral("none");
    for (const QVariant& v : config_->enabledPaymentMethods()) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("method")).toString() == method) {
            return row.value(QStringLiteral("referenceMode"),
                             QStringLiteral("none")).toString();
        }
    }
    return QStringLiteral("none");
}

bool TenderViewModel::canComplete() const {
    if (!bridge_ || !bridge_->summary()) return false;
    // Balance due <= 0 means the bill is fully tendered (over-tender → change due).
    // Parse the engine's exact decimal string; never use a double.
    const auto balance = blissmont::core::Money::parse(
        bridge_->summary()->balanceDue().toStdString());
    return balance.ok() && balance.value().minorUnits() <= 0;
}

bool TenderViewModel::autoComplete() const {
    return config_ && config_->tenderCompleteMode() == QStringLiteral("auto");
}

void TenderViewModel::addTender(const QString& method, const QString& amount,
                                const QString& reference) {
    if (!bridge_) return;
    if (referenceModeFor(method) == QStringLiteral("required") &&
        reference.trimmed().isEmpty()) {
        setStatus(tr("Reference required for %1").arg(method));
        return;  // blocked — no command sent
    }
    bridge_->addTender(method, amount, reference);
    setStatus(QString());
}

void TenderViewModel::removeTender(int tenderNo) {
    if (!bridge_) return;
    bridge_->removeTender(tenderNo);
    setStatus(QString());
}

void TenderViewModel::complete() {
    if (!bridge_) return;
    if (!canComplete()) {
        const QString due = bridge_->summary() ? bridge_->summary()->balanceDue() : QString();
        setStatus(tr("Balance due: %1").arg(due));
        return;
    }
    bridge_->settle();
}

void TenderViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
