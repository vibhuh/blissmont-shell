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
    property string notice: ""   // transient placeholder feedback (Save/Print/Misc stubs)

    readonly property bool cartActive: {
        var s = PosEngineBridge.summary.status
        return s === "active" || s === "tendering"
    }

    function focusSearch() { screen.navState = "item" }  // re-entering home re-focuses search

    // Single action handler — backs both the ActionBar buttons and Main.qml's global F-keys.
    function doAction(name) {
        switch (name) {
        case "save":    screen.notify(qsTr("Save — separate build")); break
        case "print":   screen.notify(qsTr("Print — separate build")); break
        case "hold":    screen.navState = "suspend"; break
        case "return":  screen.navState = "return"; break
        case "history": screen.navState = "history"; break
        case "misc":    screen.navState = "misc"; break   // Misc panel is a later slice
        case "clear":   PosEngineBridge.voidCart(); screen.navState = "item"; break
        case "charge":  screen.navState = "tender"; break
        case "discount": screen.navState = "billdiscount"; break
        case "customer": screen.navState = "customer"; break
        case "payout":  if (ConfigService.payoutEnabled) screen.navState = "payout"; break
        case "home":    screen.navState = "item"; break
        }
    }
    function notify(msg) { screen.notice = msg; noticeTimer.restart() }
    Timer { id: noticeTimer; interval: 2500; onTriggered: screen.notice = "" }

    // The view-model for the scan/search home-state and transient status.
    BillingViewModel {
        id: vm
        bridge: PosEngineBridge
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
                    Layout.preferredHeight: 132
                    Layout.maximumHeight: 132
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

            ItemPanel {
                Layout.preferredWidth: 320
                Layout.minimumWidth: 300
                Layout.fillHeight: true
                context: screen.navState
                vm: vm
                onNavigate: (context) => screen.navState = context
            }
        }

        // ── Transient feedback strip ──────────────────────────────────────────
        StatusLine {
            Layout.fillWidth: true
            message: screen.notice !== "" ? screen.notice : vm.statusMessage
            connectionText: ConnectionService.statusText
            online: ConnectionService.connected
        }

        // ── Zone 4: bottom action bar ─────────────────────────────────────────
        ActionBar {
            Layout.fillWidth: true
            cartActive: screen.cartActive
            allowReturns: ConfigService.allowReturns
            onTriggered: (name) => screen.doAction(name)
        }
    }
}
