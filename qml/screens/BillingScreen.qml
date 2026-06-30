import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// screens/BillingScreen.qml — the sale screen, four zones (spec):
//   1) TopBar: store · counter · bill# · online/offline · shift state
//   2) LEFT: bill grid (BillTable) over a 2-col Customer | Totals row
//   3) RIGHT (~300px): the swapping panel (ItemPanel), HOME = product search
//   4) BOTTOM: the bill-scoped, icon-only action bar
// A thin transient feedback strip (StatusLine) sits above the action bar for
// item-not-found / rejection / settle messages (and placeholder notices).
Item {
    id: screen
    property string navState: "item"
    property string notice: ""   // transient feedback (rejections, not-yet-reachable notices)
    // The receipt # of the bill settled this session — what Print reprints (brief §3). Set by
    // the orderSettled handler; gates the Print button (nothing to reprint until a sale completes).
    property string lastReceiptNo: ""

    readonly property bool cartActive: {
        var s = PosEngineBridge.summary.status
        return s === "active" || s === "tendering"
    }

    // Tier 3.3 — the status bar reports terminal STATE, not validation errors. Ready /
    // Scanning / Tender required / Sale completed (+ the connectivity dot in StatusLine).
    // Validation errors live at their field (scan not-found → the scan field; tender errors →
    // the tender panel). Transient results/notices (settle line, rejections, operational
    // hints) ride `notice` and fade back to this state.
    property bool justCompleted: false
    Timer { id: completedTimer; interval: 2500; onTriggered: screen.justCompleted = false }
    readonly property string terminalState: {
        if (screen.justCompleted)              return qsTr("Sale completed")
        if (screen.navState === "tender")      return qsTr("Tender required")
        if (vm.scanText.trim() !== "")         return qsTr("Scanning…")
        if (screen.cartActive)                 return qsTr("Building bill")
        return qsTr("Ready · scan a barcode or search")
    }

    function focusSearch() { screen.navState = "item" }  // re-entering home re-focuses search
    function openTasks() { actionBar.openTasks() }       // global-shortcut entry to the launcher

    // Single action handler — backs the ActionBar buttons, the Tasks launcher items, and
    // Main.qml's global F-keys (one path, so button and key stay in lockstep).
    function doAction(name) {
        switch (name) {
        // ── Bill-scoped action bar (brief §4) ──
        case "charge":   screen.navState = "tender"; break
        case "hold":     screen.navState = "suspend"; break
        case "discount": screen.navState = "billdiscount"; break
        case "sundry":   screen.navState = "sundry"; break
        case "print":
            if (screen.lastReceiptNo !== "") PosEngineBridge.reprintBill(screen.lastReceiptNo)
            else screen.notify(qsTr("No settled bill to print yet"))
            break
        case "clear":    PosEngineBridge.voidCart(); screen.navState = "item"; break  // Void
        case "tasks":    screen.openTasks(); break
        // ── Tasks launcher items (brief §5) ──
        case "history":  screen.navState = "history"; break
        case "payout":   if (ConfigService.payoutEnabled) screen.navState = "payout"; break
        case "cashin":   screen.navState = "cashin"; break
        case "calculator": screen.navState = "calculator"; break
        case "settings": screen.navState = "settings"; break
        case "zreport":  PosEngineBridge.runEod(); break  // engine prints the Z / closes the batch
        case "opendrawer":
            // No terminal command for a manual drawer pulse in this contract — the drawer
            // pulses with the ESC/POS receipt at settle (engine-owned). Surface that honestly.
            screen.notify(qsTr("Open Drawer — drawer pulses automatically at settle"))
            break
        case "xreport":
            // X-report is a server-side management surface; there is no terminal command for it.
            screen.notify(qsTr("X Report — view on the management surface (server-side)"))
            break
        // ── Other nav (returns seam, customer select, home) ──
        case "return":   screen.navState = "return"; break
        case "customer": screen.navState = "customer"; break
        case "home":     screen.navState = "item"; break
        }
    }
    function notify(msg) { screen.notice = msg; noticeTimer.restart() }
    Timer { id: noticeTimer; interval: 2500; onTriggered: screen.notice = "" }

    // ── Post-settle (brief §2) + EOD feedback. orderSettled fires BEFORE the engine's empty-cart
    //    reset is applied (events are delivered in order on the UI thread), so the summary still
    //    holds the just-taken tender — capture received/change here, then show the confirmation.
    Connections {
        target: PosEngineBridge
        function onOrderSettled(receiptNo, provisional, total) {
            screen.lastReceiptNo = receiptNo
            saleComplete.receiptNo = receiptNo
            saleComplete.total = total
            saleComplete.amountReceived = PosEngineBridge.summary.amountTendered
            saleComplete.changeDue = PosEngineBridge.summary.changeDue
            saleComplete.provisional = provisional
            screen.navState = "item"      // reset the right panel to product-search home
            screen.justCompleted = true   // status bar reads "Sale completed" briefly (Tier 3.3)
            completedTimer.restart()
            saleComplete.present()        // green "Sale completed" card (engine already printed)
        }
        function onEodResult(batchId, provisional) {
            screen.notify(provisional ? qsTr("Day close recorded (offline): %1").arg(batchId)
                                      : qsTr("Day close recorded: %1").arg(batchId))
        }
        function onEodBlocked(openShiftIds) {
            screen.notify(qsTr("Z Report blocked — close the open shift first (%1)")
                          .arg(openShiftIds.length))
        }
    }

    // The view-model for the scan/search home-state and transient status.
    BillingViewModel {
        id: vm
        bridge: PosEngineBridge
        config: ConfigService   // currency symbol for the settle status line
    }

    // The view-model's status messages are transient RESULTS (settle line) or generic command
    // rejections — show them briefly via the notice, then fade back to the terminal state.
    // (Field-level validation — scan not-found — never comes here; it rides vm.scanError.)
    Connections {
        target: vm
        function onStatusMessageChanged() {
            if (vm.statusMessage !== "") screen.notify(vm.statusMessage)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        // ── Zone 1: top bar ───────────────────────────────────────────────────
        TopBar {
            Layout.fillWidth: true
            online: ConnectionService.connected
            connectionText: ConnectionService.statusText
        }

        // ── Zones 2 + 3: left (bill/customer/totals) | right (swapping panel) ──
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.gap

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.gap

                BillTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: PosEngineBridge.cart
                }

                // Customer | Totals — two columns.
                // maximumHeight hard-caps this row: its children set Layout.fillHeight,
                // which makes the RowLayout inherit a fillHeight stretch that would
                // otherwise override preferredHeight and starve the BillTable above to
                // 0px. The cap keeps it a fixed bottom strip so BillTable fills the rest.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 164
                    Layout.maximumHeight: 164
                    spacing: Theme.gap
                    CustomerBlock {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        onRequestSelect: screen.navState = "customer"
                    }
                    TotalsBlock {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                    }
                }
            }

            // Right panel — the swapping WORKSPACE (search/tender/discount/history/payout),
            // not a fixed sidebar. Widened again (Tier 3.5) so the search field, chips,
            // quick-key rows, the tender calculator and touch targets all breathe; the cart
            // grid still dominates the larger left column (this is workspace, never wasted space).
            ItemPanel {
                Layout.preferredWidth: 500
                Layout.minimumWidth: 420
                Layout.fillHeight: true
                context: screen.navState
                vm: vm
                onNavigate: (context) => screen.navState = context
            }
        }

        // ── Transient feedback strip ──────────────────────────────────────────
        StatusLine {
            Layout.fillWidth: true
            // State-first (Tier 3.3): a transient notice (settle result / rejection / op hint)
            // overlays the steady terminal state, then fades back to it.
            message: screen.notice !== "" ? screen.notice : screen.terminalState
            connectionText: ConnectionService.statusText
            online: ConnectionService.connected
        }

        // ── Zone 4: bottom action bar ─────────────────────────────────────────
        ActionBar {
            id: actionBar
            Layout.fillWidth: true
            cartActive: screen.cartActive
            canReprint: screen.lastReceiptNo !== ""
            onTriggered: (name) => screen.doAction(name)
        }
    }

    // ── Post-settle confirmation overlay (brief §2) ──────────────────────────────
    // Sits above every zone; shown by the orderSettled handler. On dismiss it re-focuses the
    // scan field so the cashier rings the next customer immediately.
    SaleCompleteOverlay {
        id: saleComplete
        anchors.fill: parent
        onDismissed: screen.focusSearch()
    }
}
