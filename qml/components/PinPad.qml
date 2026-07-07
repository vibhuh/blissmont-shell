import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/PinPad.qml — numeric PIN entry for the Begin-Day operator login (Slice B).
//
// A masked-dots display over a 3×4 digit grid. Keyboard-first: when it holds focus it
// also accepts hardware / number-pad digit keys, Backspace/Delete, and Enter (which
// fires `submitted` once the PIN reaches minLength). Escape is left unhandled so it
// bubbles to the workflow (cancel). The PIN itself never leaves this component except
// via the `pin` property the parent reads at open time — it is never logged.
Item {
    id: pad

    property int    minLength: 4
    property int    maxLength: 8
    property string pin: ""
    readonly property bool complete: pad.pin.length >= pad.minLength
    signal submitted()

    function input(d) { if (pad.pin.length < pad.maxLength) pad.pin += d }
    function backspace() { if (pad.pin.length > 0) pad.pin = pad.pin.slice(0, pad.pin.length - 1) }
    function clearAll() { pad.pin = "" }

    implicitWidth: 260
    implicitHeight: layout.implicitHeight

    focus: true
    Keys.onPressed: (e) => {
        if (e.key >= Qt.Key_0 && e.key <= Qt.Key_9) { pad.input(String(e.key - Qt.Key_0)); e.accepted = true }
        else if (e.key === Qt.Key_Backspace || e.key === Qt.Key_Delete) { pad.backspace(); e.accepted = true }
        else if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter) { if (pad.complete) pad.submitted(); e.accepted = true }
    }

    component PadKey: Button {
        id: keyBtn
        property string label: ""
        property bool subtle: false
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.touchMin
        contentItem: Text {
            text: keyBtn.label
            color: keyBtn.subtle ? Theme.textMuted : Theme.text
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: keyBtn.down ? Theme.selectionBg : Theme.surfaceAlt
            border.color: Theme.border
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Theme.gap

        // Masked dots — one filled dot per entered digit, minLength placeholders.
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.unit
            Repeater {
                model: Math.max(pad.pin.length, pad.minLength)
                delegate: Rectangle {
                    required property int index
                    width: 14; height: 14; radius: 7
                    color: index < pad.pin.length ? Theme.accent : "transparent"
                    border.color: Theme.border; border.width: 1
                }
            }
        }

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 260
            columns: 3
            rowSpacing: Theme.unit
            columnSpacing: Theme.unit
            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9"]
                delegate: PadKey {
                    required property var modelData
                    label: modelData
                    onClicked: pad.input(modelData)
                }
            }
            PadKey { label: "⌫"; subtle: true; onClicked: pad.backspace() }
            PadKey { label: "0"; onClicked: pad.input("0") }
            PadKey { label: "C"; subtle: true; onClicked: pad.clearAll() }
        }
    }
}
