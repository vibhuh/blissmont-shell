import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/PayoutPanel.qml — the payout context (UX §12), swapped into ItemPanel on Ctrl+O.
// Cash out of the drawer to a categorised destination. The cashier picks a category (the
// server-resolved keys from ConfigService.payoutCategories — the category→GL-account mapping
// stays server-side), enters an amount and an optional note; the engine + server own the GL
// posting (a balanced journal by category, rachis-core ADR-0008/0009). Pure presentation: this
// panel only collects input, dispatches, and renders the engine's confirmation/reject. Gated
// upstream by Ctrl+O on payout_enabled; the empty-state here is the defensive case where payouts
// are enabled but the store has no categories configured.
Item {
    id: root

    PayoutViewModel {
        id: pvm
        bridge: PosEngineBridge
        onRecorded: { amountField.clear(); noteField.clear() }
    }

    readonly property bool hasCategories: ConfigService.payoutCategories.length > 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Payout")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // ── Category (required — the server resolves it to a GL account) ───────
        Text {
            text: qsTr("Category")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        ComboBox {
            id: categoryBox
            Layout.fillWidth: true
            model: ConfigService.payoutCategories
            enabled: root.hasCategories
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }

        // ── Amount ────────────────────────────────────────────────────────────
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
            onAccepted: confirm.clicked()
        }

        // ── Note (optional) ───────────────────────────────────────────────────
        Text {
            text: qsTr("Note (optional)")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        TextField {
            id: noteField
            Layout.fillWidth: true
            placeholderText: qsTr("e.g. supplier invoice #")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            color: Theme.text
            placeholderTextColor: Theme.textMuted
        }

        Button {
            id: confirm
            text: qsTr("Record payout")
            enabled: root.hasCategories
            onClicked: pvm.recordPayout(amountField.text, categoryBox.currentText, noteField.text)
        }

        // Defensive empty-state: payouts enabled but no categories configured for the store.
        Text {
            Layout.fillWidth: true
            visible: !root.hasCategories
            text: qsTr("No payout categories are configured for this store.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }

        // Engine confirmation ("Paid out: …") or reject, display-only.
        Text {
            Layout.fillWidth: true
            text: pvm.statusMessage
            visible: pvm.statusMessage !== ""
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
