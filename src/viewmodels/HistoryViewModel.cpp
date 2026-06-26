// viewmodels/HistoryViewModel.cpp — see HistoryViewModel.hpp.
#include "viewmodels/HistoryViewModel.hpp"

#include "models/BillDetailModel.hpp"

namespace blissmont::viewmodels {

HistoryViewModel::HistoryViewModel(QObject* parent) : QObject(parent) {}

void HistoryViewModel::setBridge(blissmont::bridge::PosEngineBridge* bridge) {
    if (bridge_ == bridge) return;
    bridge_ = bridge;
    if (bridge_) {
        using B = blissmont::bridge::PosEngineBridge;
        // The detail model drives detailActive AND the reprint-echo handling — a recall fills
        // it (→ detail), a reprint-from-list fills then clears it (→ stay on list).
        if (bridge_->billDetail()) {
            connect(bridge_->billDetail(), &blissmont::models::BillDetailModel::changed, this,
                    &HistoryViewModel::onDetailChanged);
        }
        // Engine rejects (e.g. NOT_FOUND on an unknown receipt) surface verbatim.
        connect(bridge_, &B::commandRejected, this,
                [this](const QString& code, const QString& message) {
                    pendingReprint_ = false;  // a failed reprint is not a reprint
                    setStatus(message.isEmpty() ? code : message);
                });
    }
    emit bridgeChanged();
    emit stateChanged();
}

void HistoryViewModel::setConnection(blissmont::services::ConnectionService* connection) {
    if (connection_ == connection) return;
    connection_ = connection;
    if (connection_) {
        // engineOnline can flip on any SyncStatusChanged → re-evaluate the local-only hint.
        connect(connection_, &blissmont::services::ConnectionService::changed, this,
                &HistoryViewModel::stateChanged);
    }
    emit connectionChanged();
    emit stateChanged();
}

bool HistoryViewModel::detailActive() const {
    return bridge_ && bridge_->billDetail() && bridge_->billDetail()->active();
}

bool HistoryViewModel::localOnlyHint() const {
    // Local-only whenever the engine isn't online to the server (offline, or no engine yet).
    return !connection_ || !connection_->engineOnline();
}

void HistoryViewModel::onDetailChanged() {
    // Fired whenever the detail model resets. A reprint's echoed BillDetail is a confirmation,
    // not a view request: show the DUPLICATE status, and if the reprint came from the list,
    // drop the echo so the panel stays on the list (clear() re-enters here with no detail).
    if (pendingReprint_ && detailActive()) {
        pendingReprint_ = false;
        const QString receiptNo = bridge_->billDetail()->receiptNo();
        setStatus(tr("Reprinted %1 as DUPLICATE").arg(receiptNo));
        if (reprintKeepList_) {
            bridge_->billDetail()->clear();
            return;  // the clear's re-entry flips detailActive false; emit once below
        }
    }
    emit stateChanged();
}

void HistoryViewModel::open(int limit) {
    if (!bridge_) return;
    if (bridge_->billDetail()) bridge_->billDetail()->clear();  // start on the list
    setStatus(QString());
    bridge_->recallRecent(limit > 0 ? limit : kDefaultLimit);
}

void HistoryViewModel::openDetail(const QString& receiptNo) {
    if (!bridge_ || receiptNo.trimmed().isEmpty()) return;
    setStatus(QString());
    bridge_->recallByReceiptNo(receiptNo.trimmed());
}

void HistoryViewModel::searchByReceiptNo(const QString& receiptNo) {
    openDetail(receiptNo);  // exact recall opens the bill's detail
}

void HistoryViewModel::searchByCustomer(const QString& query) {
    if (!bridge_) return;
    if (bridge_->billDetail()) bridge_->billDetail()->clear();  // results show in the list
    setStatus(QString());
    bridge_->searchByCustomer(query.trimmed());
}

void HistoryViewModel::reprint(const QString& receiptNo) {
    if (!bridge_ || receiptNo.trimmed().isEmpty()) return;
    pendingReprint_ = true;
    // Launched from the list (no detail showing) → drop the echoed detail to stay on the list;
    // launched from detail → keep it (same bill, view unchanged).
    reprintKeepList_ = !detailActive();
    bridge_->reprintBill(receiptNo.trimmed());
}

void HistoryViewModel::startReturn(const QString& receiptNo) {
    if (!bridge_ || receiptNo.trimmed().isEmpty()) return;
    bridge_->startReturn(receiptNo.trimmed(), /*blind=*/false);
    emit returnRequested();
}

void HistoryViewModel::closeDetail() {
    if (bridge_ && bridge_->billDetail()) bridge_->billDetail()->clear();
}

void HistoryViewModel::setStatus(const QString& message) {
    if (statusMessage_ == message) return;
    statusMessage_ = message;
    emit statusMessageChanged();
}

}  // namespace blissmont::viewmodels
