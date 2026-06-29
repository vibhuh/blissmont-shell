import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// components/SaleCompleteOverlay.qml — the post-settle confirmation (brief §2). The single
// most important "the sale is done" signal: a green success card shown within ~1s of settle
// with the invoice/receipt number prominent and the amount received + change due from the
// tender just taken. It does NOT compute or print anything — the engine auto-prints the
// receipt at settle and owns all money; this only reflects the OrderSettled snapshot the host
// captured. It auto-dismisses (the cashier is freed for the next customer) and dismisses on
// any key / tap, raising `dismissed` so the host can re-focus the scan field.
Item {
    id: overlay

    // Captured from the host at the instant orderSettled fired (before the empty-cart reset),
    // so these are the values of the bill just completed.
    property string receiptNo: ""
    property string total: "0.00"
    property string amountReceived: "0.00"
    property string changeDue: "0.00"
    property bool   provisional: false
    readonly property string sym: ConfigService.currencySymbol

    signal dismissed()

    visible: false
    function present() {
        visible = true
        dismissTimer.restart()
        card.forceActiveFocus()   // so any key dismisses
    }
    function dismiss() {
        if (!visible) return
        dismissTimer.stop()
        visible = false
        overlay.dismissed()
    }

    // ~4s is long enough to read the receipt # and change, short enough to not block the
    // next sale; any key or tap dismisses sooner.
    Timer { id: dismissTimer; interval: 4000; onTriggered: overlay.dismiss() }

    // Dim backdrop — tap anywhere to dismiss and ready the next sale.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.45
        TapHandler { onTapped: overlay.dismiss() }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.6, 520)
        implicitHeight: content.implicitHeight + 2 * Theme.pad
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.ok
        border.width: 2
        focus: overlay.visible
        Keys.onPressed: (event) => { overlay.dismiss(); event.accepted = true }

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: Theme.pad
            spacing: Theme.gap

            // ── Success heading (green/success treatment) ─────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                Text {
                    text: "✔"   // ✔
                    color: Theme.ok
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTotal
                }
                Text {
                    Layout.fillWidth: true
                    text: overlay.provisional ? qsTr("Sale completed (offline)")
                                              : qsTr("Sale completed")
                    color: Theme.ok
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTotal
                    font.bold: true
                }
            }

            // ── Invoice / receipt number, prominent ───────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: qsTr("Invoice")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                }
                Text {
                    Layout.fillWidth: true
                    text: overlay.receiptNo
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontTotal
                    elide: Text.ElideRight
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            // ── Amount received + change due (from the tender just taken) ──────────
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.gap
                rowSpacing: Theme.unit

                Text { text: qsTr("Total"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                Text { text: overlay.sym + overlay.total; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }

                Text { text: qsTr("Received"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                Text { text: overlay.sym + overlay.amountReceived; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }

                Text {
                    text: qsTr("Change")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontLarge
                    visible: changeValue.visible
                }
                Text {
                    id: changeValue
                    text: overlay.sym + overlay.changeDue
                    color: Theme.ok
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontLarge
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    // Hide the change row for an exact tender (no change due).
                    visible: overlay.changeDue !== "" && overlay.changeDue !== "0.00"
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Press any key to start the next sale")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
