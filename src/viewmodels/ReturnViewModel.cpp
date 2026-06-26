// viewmodels/ReturnViewModel.cpp — see ReturnViewModel.hpp.
#include "viewmodels/ReturnViewModel.hpp"

#include "models/ReturnLineModel.hpp"

namespace blissmont::viewmodels {

ReturnViewModel::ReturnViewModel(QObject* parent) : QObject(parent) {}

void ReturnViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    if (bridge_) {
        using B = blissmont::bridge::PosEngineBridge;
        // A re-emitted return context (start OR each line edit) replaces the model; the
        // selection that drives canCommit rides those resets.
        if (bridge_->returnLines()) {
            connect(bridge_->returnLines(), &QAbstractItemModel::modelReset, this,
                    &ReturnViewModel::stateChanged);
        }
        connect(bridge_, &B::returnContextLoaded, this, [this](const QString& receiptNo) {
            originalReceiptNo_ = receiptNo;
            setStatus(QString());
            emit stateChanged();
        });
        connect(bridge_, &B::returnCommitted, this,
                [this](const QString& creditNoteNo, bool provisional, const QString& total,
                       const QString& taxReversed) {
                    originalReceiptNo_.clear();
                    setStatus(tr("Credit note %1%2 — total %3, tax reversed %4")
                                  .arg(creditNoteNo, provisional ? tr(" (provisional)") : QString(),
                                       total, taxReversed));
                    emit stateChanged();
                });
        // Canonical reconcile on sync. Display-only: it must NEVER gate print/navigation or
        // the next return — it only updates the on-screen status (two-event lifecycle).
        connect(bridge_, &B::refundSettled, this,
                [this](const QString& refundNo, bool provisional, const QString& total) {
                    if (provisional) return;  // the provisional case is ReturnCommitted's job
                    setStatus(tr("Refund %1 settled — total %2").arg(refundNo, total));
                });
        // Supervisor gate + engine rejects (e.g. REFUND_MODE_NOT_SUPPORTED,
        // PARTIAL_RETURN_NOT_ALLOWED) surface verbatim — the engine owns the rules.
        connect(bridge_, &B::authRequired, this,
                [this](const QString& action, const QString& reason) {
                    if (action == QStringLiteral("return")) setStatus(reason);
                });
        connect(bridge_, &B::commandRejected, this,
                [this](const QString& code, const QString& message) {
                    setStatus(message.isEmpty() ? code : message);
                });
    }
    emit bridgeChanged();
    emit stateChanged();
}

void ReturnViewModel::setConfig(blissmont::services::ConfigService* config) {
    if (config_ == config) return;
    config_ = config;
    if (config_) {
        // refundTenderMode / allowBlindReturn can change on a config push.
        connect(config_, &blissmont::services::ConfigService::changed, this,
                &ReturnViewModel::stateChanged);
    }
    emit configChanged();
    emit stateChanged();
}

bool ReturnViewModel::active() const {
    return bridge_ && bridge_->returnLines() && bridge_->returnLines()->rowCount() > 0;
}

bool ReturnViewModel::refundModeSupported() const {
    // Phase A: "both" (split original + cash) is not yet wired — the engine rejects commit
    // with REFUND_MODE_NOT_SUPPORTED, so the panel must not offer it.
    return !config_ || config_->refundTenderMode() != QStringLiteral("both");
}

bool ReturnViewModel::allowBlind() const {
    return config_ && config_->allowBlindReturn();
}

bool ReturnViewModel::hasSelectedLines() const {
    if (!bridge_ || !bridge_->returnLines()) return false;
    auto* model = bridge_->returnLines();
    using R = blissmont::models::ReturnLineModel;
    for (int i = 0; i < model->rowCount(); ++i) {
        const QString sel = model->data(model->index(i, 0), R::SelectedQtyRole).toString();
        // selectedQty is a fixed-decimal QUANTITY string (e.g. "1.0000000000"), not money —
        // don't route it through core/Money (2-decimal currency). It is positive iff it
        // carries any non-zero digit (quantities are non-negative; the engine rejects < 0).
        for (const QChar c : sel) {
            if (c.isDigit() && c != QLatin1Char('0')) return true;
        }
    }
    return false;
}

bool ReturnViewModel::canCommit() const {
    return active() && refundModeSupported() && hasSelectedLines();
}

void ReturnViewModel::startReturn(const QString& receiptNo, bool blind) {
    if (!bridge_) return;
    if (blind && !allowBlind()) {
        setStatus(tr("Blind returns are not permitted on this terminal"));
        return;  // blocked — no command sent
    }
    bridge_->startReturn(receiptNo, blind);
    setStatus(QString());
}

void ReturnViewModel::setLineQty(int originalLineNo, const QString& qty, bool restock) {
    if (!bridge_) return;
    bridge_->setReturnLineQty(originalLineNo, qty, restock);
}

void ReturnViewModel::commit() {
    if (!bridge_) return;
    if (!refundModeSupported()) {
        setStatus(tr("Split refund (original + cash) is not yet supported on this terminal"));
        return;  // blocked — Phase A safety, mirrors the engine reject
    }
    if (!canCommit()) {
        setStatus(tr("Select at least one line to return"));
        return;
    }
    bridge_->commitReturn();
}

void ReturnViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
