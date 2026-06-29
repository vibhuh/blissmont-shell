import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/SundryPanel.qml — the Sundry (manual non-product charge) takeover (brief §4, UX §11).
// Pairs with the bill discount: Discount reduces the bill, Sundry ADDS a manual charge line
// (delivery / packing / manual charge). Opens in the RIGHT panel (swap, not modal); restores
// the search home-state on Add / Cancel. The engine applies tax from the config's
// misc_charge_tax_mode (never the UI) and marks the line discount_eligible=false (ADR-0009 — a
// sundry charge is not discountable); this panel only collects a description + amount and
// dispatches add_misc_charge. The new line lands via the next CartUpdated snapshot.
Item {
    id: panel
    signal done()
    readonly property string sym: ConfigService.currencySymbol

    function add() {
        var desc = descField.text.trim()
        var amt = amountField.text.trim()
        if (desc === "" || amt === "") return
        PosEngineBridge.addMiscCharge(desc, amt)
        panel.done()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Sundry charge")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // ── Description (required) ────────────────────────────────────────────
        Text {
            text: qsTr("Description")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        TextField {
            id: descField
            Layout.fillWidth: true
            placeholderText: qsTr("e.g. Delivery, Packing")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            color: Theme.text
            placeholderTextColor: Theme.textMuted
            Component.onCompleted: forceActiveFocus()
            onAccepted: amountField.forceActiveFocus()
        }

        // ── Amount (required, positive) ───────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text { text: qsTr("Amount"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
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
                Keys.onReturnPressed: panel.add()
                Keys.onEnterPressed: panel.add()
            }
            Text { text: panel.sym; color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Tax applies per store config. Not eligible for bill discount.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Button {
                Layout.fillWidth: true
                text: qsTr("Cancel")
                onClicked: panel.done()
            }
            Button {
                id: addBtn
                Layout.fillWidth: true
                text: qsTr("Add charge")
                enabled: descField.text.trim() !== "" && amountField.text.trim() !== ""
                onClicked: panel.add()
                contentItem: Text {
                    text: addBtn.text
                    color: addBtn.enabled ? Theme.bg : Theme.textMuted
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radius
                    color: addBtn.enabled ? Theme.accent : Theme.surface
                    border.color: addBtn.enabled ? Theme.accent : Theme.border
                }
            }
        }
    }
}
