// viewmodels/TenderViewModel.cpp — see TenderViewModel.hpp.
#include "viewmodels/TenderViewModel.hpp"

#include "core/Format.hpp"
#include "core/Money.hpp"

namespace blissmont::viewmodels {

TenderViewModel::TenderViewModel(QObject* parent) : QObject(parent) {}

void TenderViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    // The balance is derived from the engine's CartSummary; re-evaluate completion (and
    // fire an armed settle) whenever a fresh snapshot lands.
    if (bridge_ && bridge_->summary()) {
        connect(bridge_->summary(), &blissmont::models::CartSummary::changed, this,
                &TenderViewModel::onBalanceChanged);
    }
    emit bridgeChanged();
    emit stateChanged();
}

void TenderViewModel::onBalanceChanged() {
    emit stateChanged();
    if (completeArmed_ && canComplete()) {
        completeArmed_ = false;
        bridge_->settle();
    }
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

QString TenderViewModel::primaryMethod() const {
    if (!config_) return QStringLiteral("cash");
    QString first;
    for (const QVariant& v : config_->enabledPaymentMethods()) {
        const QString m = v.toMap().value(QStringLiteral("method")).toString();
        if (first.isEmpty()) first = m;
        if (m.compare(QStringLiteral("cash"), Qt::CaseInsensitive) == 0) return m;
    }
    return first.isEmpty() ? QStringLiteral("cash") : first;
}

QString TenderViewModel::changeDuePreview(const QString& received) const {
    if (!bridge_ || !bridge_->summary()) return QStringLiteral("0.00");
    // Round both to 2 dp first: the engine's total is a full-precision string, and the
    // received field may be mid-keystroke ("600.567"); core::Money::parse takes <=2 dp.
    const auto total = blissmont::core::Money::parse(
        blissmont::core::numfmt::round(bridge_->summary()->total().toStdString(), 2));
    const auto recv = blissmont::core::Money::parse(
        blissmont::core::numfmt::round(received.trimmed().toStdString(), 2));
    if (!total.ok() || !recv.ok()) return QStringLiteral("0.00");
    const auto change = recv.value() - total.value();
    if (change.minorUnits() <= 0) return QStringLiteral("0.00");  // no change when short/exact
    return QString::fromStdString(change.toString());
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

void TenderViewModel::tenderAndComplete(const QString& method, const QString& amount,
                                       const QString& reference) {
    if (!bridge_) return;
    if (referenceModeFor(method) == QStringLiteral("required") &&
        reference.trimmed().isEmpty()) {
        setStatus(tr("Reference required for %1").arg(method));
        return;  // blocked — no command sent, completion NOT armed
    }
    // Normalize the amount to a bare 2-dp decimal the engine can parse (the field may hold
    // "600.5" or "600.567"); arm the settle, then tender. onBalanceChanged fires the settle
    // as soon as the engine reports the balance cleared.
    const QString clean =
        QString::fromStdString(blissmont::core::numfmt::round(amount.toStdString(), 2));
    completeArmed_ = true;
    bridge_->addTender(method, clean, reference);
    setStatus(QString());
}

void TenderViewModel::removeTender(int tenderNo) {
    if (!bridge_) return;
    completeArmed_ = false;  // cashier is adjusting the split — don't auto-settle
    bridge_->removeTender(tenderNo);
    setStatus(QString());
}

void TenderViewModel::complete() {
    if (!bridge_) return;
    if (!canComplete()) {
        const QString raw = bridge_->summary() ? bridge_->summary()->balanceDue() : QString();
        const QString symbol =
            config_ ? config_->currencySymbol() : QStringLiteral("₹");
        const QString due =
            symbol + QStringLiteral(" ") +
            QString::fromStdString(blissmont::core::numfmt::money(raw.toStdString()));
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
