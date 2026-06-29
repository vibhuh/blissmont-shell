// viewmodels/BillingViewModel.cpp — see BillingViewModel.hpp.
#include "viewmodels/BillingViewModel.hpp"

#include "core/Format.hpp"

namespace blissmont::viewmodels {

BillingViewModel::BillingViewModel(QObject* parent) : QObject(parent) {}

void BillingViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    wireBridge();
    emit bridgeChanged();
}

void BillingViewModel::setConfig(blissmont::services::ConfigService* config) {
    if (config_ == config) return;
    config_ = config;
    emit configChanged();
}

QString BillingViewModel::currencySymbol() const {
    return config_ ? config_->currencySymbol() : QStringLiteral("₹");
}

void BillingViewModel::wireBridge() {
    if (!bridge_) return;
    using B = blissmont::bridge::PosEngineBridge;
    connect(bridge_, &B::itemNotFound, this, [this](const QString& barcode) {
        setStatus(QStringLiteral("Item not found: %1").arg(barcode));
    });
    connect(bridge_, &B::commandRejected, this, [this](const QString&, const QString& message) {
        setStatus(message);
    });
    connect(bridge_, &B::orderSettled, this,
            [this](const QString& receiptNo, bool provisional, const QString& total) {
                // Route the engine's exact decimal string through the one formatter — the
                // status line must never show raw precision ("1003.0000000000").
                const QString amount =
                    currencySymbol() +
                    QString::fromStdString(blissmont::core::numfmt::money(total.toStdString()));
                setStatus(provisional
                              ? QStringLiteral("Settled (offline) %1 — %2").arg(receiptNo, amount)
                              : QStringLiteral("Settled %1 — %2").arg(receiptNo, amount));
            });
}

void BillingViewModel::setScanText(const QString& text) {
    if (scanText_ == text) return;
    scanText_ = text;
    emit scanTextChanged();
}

void BillingViewModel::submitScan() {
    const QString code = scanText_.trimmed();
    if (code.isEmpty() || !bridge_) return;
    bridge_->scanItem(code);
    setScanText(QString());  // clear -> focus returns to empty scan field (home law)
}

void BillingViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
