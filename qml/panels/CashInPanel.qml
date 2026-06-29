import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/CashInPanel.qml — the Cash In takeover (Tasks launcher, UX §12). Adds cash to the
// drawer (a float top-up / change order), the counterpart to Payout. Dispatches the existing
// record_cash_movement command with type "cash_in"; the engine + server own the GL posting
// (Dr cash / Cr the resolved account). Pure presentation: collect amount + reason, dispatch,
// render the engine's CashMovementRecorded confirmation or its reject. The engine owns money —
// the amount echoed back is exactly what it accepted, never re-keyed here.
Item {
    id: panel
    signal done()
    readonly property string sym: ConfigService.currencySymbol
    property string statusMessage: ""

    function record() {
        var amt = amountField.text.trim()
        if (amt === "") return
        panel.statusMessage = ""
        PosEngineBridge.recordCashMovement("cash_in", amt, reasonField.text.trim())
    }

    Connections {
        target: PosEngineBridge
        function onCashMovementRecorded(movementId, type, amount) {
            if (type !== "cash_in") return
            panel.statusMessage = qsTr("Cash in recorded: %1%2").arg(panel.sym).arg(amount)
            amountField.clear()
            reasonField.clear()
        }
        function onCommandRejected(code, message) {
            panel.statusMessage = message
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Cash in")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        Text {
            text: qsTr("Amount")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        TextField {
            id: amountField
            Layout.fillWidth: true
            placeholderText: qsTr("0.00")
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: 0
                decimals: 2
                notation: DoubleValidator.StandardNotation
            }
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontBody
            color: Theme.text
            placeholderTextColor: Theme.textMuted
            Component.onCompleted: forceActiveFocus()
            onAccepted: panel.record()
        }

        Text {
            text: qsTr("Reason (optional)")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        TextField {
            id: reasonField
            Layout.fillWidth: true
            placeholderText: qsTr("e.g. opening float top-up")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            color: Theme.text
            placeholderTextColor: Theme.textMuted
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Button {
                Layout.fillWidth: true
                text: qsTr("Done")
                onClicked: panel.done()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Record cash in")
                enabled: amountField.text.trim() !== ""
                onClicked: panel.record()
            }
        }

        Item { Layout.fillHeight: true }

        // Engine confirmation ("Cash in recorded: …") or reject, display-only.
        Text {
            Layout.fillWidth: true
            text: panel.statusMessage
            visible: panel.statusMessage !== ""
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
