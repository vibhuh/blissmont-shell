// viewmodels/HistoryViewModel.hpp — history-panel presentation logic (history shell build).
//
// The one piece of real history logic in the shell. Holds NO bill state: the engine owns it
// and the panel binds the recent rows / detail directly to the bridge's HistoryListModel /
// BillDetailModel. Responsibilities: dispatch the four local-first reads (recall recent,
// recall by receipt, search by customer, reprint), drive the list ↔ detail toggle off the
// detail model, fold a reprint into a status line (the DUPLICATE banner is engine-side — the
// event carries no flag, so reprint is tracked by intent), hand a recalled receipt to the
// already-wired returns flow, and surface a local-only hint when the engine is offline (the
// contract has no completeness signal — see HISTORY brief §4). No QML dependency, so it is
// unit-tested headless against a bridge + ConnectionService.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"
#include "services/ConnectionService.hpp"

namespace blissmont::viewmodels {

class HistoryViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    Q_PROPERTY(blissmont::services::ConnectionService* connection READ connection WRITE setConnection NOTIFY connectionChanged)
    // True while a recalled bill's detail is shown (the bridge's BillDetailModel holds a bill).
    // The panel toggles list ↔ detail on this.
    Q_PROPERTY(bool detailActive READ detailActive NOTIFY stateChanged)
    // True when history results are local-only because the engine can't reach the server:
    // older bills / other-terminal bills won't be found. Shell-side hint — the contract
    // raises no completeness signal.
    Q_PROPERTY(bool localOnlyHint READ localOnlyHint NOTIFY stateChanged)
    // Rejects (e.g. engine NOT_FOUND on an unknown receipt) and the reprint confirmation.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit HistoryViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] blissmont::services::ConnectionService* connection() const { return connection_; }
    void setConnection(blissmont::services::ConnectionService* connection);

    [[nodiscard]] bool detailActive() const;
    [[nodiscard]] bool localOnlyHint() const;
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

    // Open the panel: recall the most recent finalized bills (recent-list-first). Drops any
    // showing detail so the panel starts on the list. limit <= 0 falls back to the default.
    Q_INVOKABLE void open(int limit = kDefaultLimit);
    // Recall one bill by exact receipt/invoice number → opens its detail (view-only).
    Q_INVOKABLE void openDetail(const QString& receiptNo);
    // Search older bills by exact receipt number (alias of openDetail — exact recall opens
    // detail) and by customer (fills the recent list).
    Q_INVOKABLE void searchByReceiptNo(const QString& receiptNo);
    Q_INVOKABLE void searchByCustomer(const QString& query);
    // Reprint a bill as a DUPLICATE (engine marks the printed bytes). Does not navigate into
    // detail when invoked from the list — the echoed BillDetail is dropped to stay on the list.
    Q_INVOKABLE void reprint(const QString& receiptNo);
    // Start a return from a recalled bill — the seam to the existing returns flow. A recalled
    // finalized bill is never blind. Emits returnRequested() so the host can flip to "return".
    Q_INVOKABLE void startReturn(const QString& receiptNo);
    // Close the detail view, back to the recent list.
    Q_INVOKABLE void closeDetail();

    static constexpr int kDefaultLimit = 20;

signals:
    void bridgeChanged();
    void connectionChanged();
    void stateChanged();
    void statusMessageChanged();
    // Emitted when startReturn dispatches — the host (QML) flips navState to "return" so the
    // already-built ReturnPanel/ReturnViewModel take over the recalled receipt.
    void returnRequested();

private:
    void onDetailChanged();
    void setStatus(const QString& message);

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    blissmont::services::ConnectionService* connection_ = nullptr;
    // A reprint is in flight; its echoed BillDetail is a confirmation, not a view request.
    bool pendingReprint_ = false;
    // The in-flight reprint was launched from the list (not detail) → drop the echo to stay.
    bool reprintKeepList_ = false;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
