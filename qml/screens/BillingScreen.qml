import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// screens/BillingScreen.qml — the one screen in the walking skeleton (spec §6).
// Left: the bill (BillTable bound to the engine's CartLineModel + totals footer).
// Right: the persistent context panel (35-40% width, swapped by navState).
// Top: the home scan field. Bottom: the status line.
Item {
    id: screen
    property string navState: "item"
    function focusScan() { scan.forceActiveFocus() }

    // The only view-model on this screen; binds to the bridge singleton.
    BillingViewModel {
        id: vm
        bridge: PosEngineBridge
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        ScanField {
            id: scan
            Layout.fillWidth: true
            text: vm.scanText
            onTextEdited: vm.scanText = text
            onSubmit: vm.submitScan()
            Component.onCompleted: forceActiveFocus()  // scan-is-home (UX §3)
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.gap

            // ── Left: the bill ────────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.unit

                BillTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: PosEngineBridge.cart
                }

                // Totals footer — bound to the engine's CartSummary (exact strings).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: Theme.surface
                    radius: Theme.radius
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.pad
                        Text {
                            text: PosEngineBridge.summary.customerLabel || qsTr("Walk-in")
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: qsTr("Total ")
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontLarge
                        }
                        Text {
                            text: PosEngineBridge.summary.total || "0.00"
                            color: Theme.text
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontTotal
                        }
                    }
                }
            }

            // ── Right: persistent context panel (spec §5) ─────────────────────
            ItemPanel {
                Layout.preferredWidth: Math.round(screen.width * 0.38)
                Layout.fillHeight: true
                context: screen.navState
                // A panel can request a context switch (e.g. history → return seam).
                onNavigate: (context) => screen.navState = context
            }
        }

        StatusLine {
            Layout.fillWidth: true
            message: vm.statusMessage
            connectionText: ConnectionService.statusText
            online: ConnectionService.connected
        }
    }
}
