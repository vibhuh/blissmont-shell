import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/BillDiscountPanel.qml — the bill-level discount takeover (spec). Opens in the
// RIGHT panel (swap, not modal); restores the search home-state on Apply/Cancel. Both
// modes converge to the same engine input: FLAT sends the rupee amount; PERCENTAGE sends
// "<value>%" and the engine computes the rupee amount, then both follow the identical
// before-tax apportionment path (Hamilton over the ELIGIBLE lines only). The engine owns
// all the money — this panel never apportions or computes tax.
Item {
    id: panel
    signal done()

    property string mode: "percent"   // "percent" | "flat"
    readonly property string sym: ConfigService.currencySymbol

    function apply() {
        var v = valueField.text.trim()
        if (v === "") return
        // FLAT → rupee amount; PERCENT → "<v>%" (engine resolves against the eligible base).
        PosEngineBridge.setOrderDiscount(panel.mode === "flat" ? v : v + "%")
        panel.done()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Bill discount")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // Mode: Percentage / Flat amount
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            RadioButton {
                id: pctRadio
                checked: panel.mode === "percent"
                onClicked: panel.mode = "percent"
                contentItem: Text { text: qsTr("Percentage"); color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; leftPadding: pctRadio.indicator.width + Theme.unit; verticalAlignment: Text.AlignVCenter }
            }
            RadioButton {
                id: flatRadio
                checked: panel.mode === "flat"
                onClicked: panel.mode = "flat"
                contentItem: Text { text: qsTr("Flat amount"); color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; leftPadding: flatRadio.indicator.width + Theme.unit; verticalAlignment: Text.AlignVCenter }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text { text: qsTr("Value"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
            TextField {
                id: valueField
                Layout.fillWidth: true
                placeholderText: panel.mode === "flat" ? qsTr("amount") : qsTr("percent")
                font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                color: Theme.text; placeholderTextColor: Theme.textMuted
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                Keys.onReturnPressed: panel.apply()
                Keys.onEnterPressed: panel.apply()
                Component.onCompleted: forceActiveFocus()
            }
            Text {
                text: panel.mode === "flat" ? panel.sym : "%"
                color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
            }
        }

        // Preview. Flat resolves to a rupee figure here; percent resolves engine-side
        // (against the eligible base), so we state the rule rather than invent a number.
        Text {
            Layout.fillWidth: true
            text: valueField.text.trim() === ""
                  ? qsTr("Enter a value")
                  : (panel.mode === "flat"
                     ? qsTr("Bill discount  %1%2").arg(panel.sym).arg(valueField.text.trim())
                     : qsTr("%1% off eligible lines").arg(valueField.text.trim()))
            color: Theme.textMuted
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Button {
                Layout.fillWidth: true
                text: qsTr("Cancel")
                onClicked: panel.done()
            }
            Button {
                id: applyBtn
                Layout.fillWidth: true
                text: qsTr("Apply")
                enabled: valueField.text.trim() !== ""
                onClicked: panel.apply()
                contentItem: Text {
                    text: applyBtn.text
                    color: applyBtn.enabled ? Theme.bg : Theme.textMuted
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radius
                    color: applyBtn.enabled ? Theme.accent : Theme.surface
                    border.color: applyBtn.enabled ? Theme.accent : Theme.border
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
