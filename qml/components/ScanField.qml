import QtQuick
import QtQuick.Controls.Basic
import Blissmont.Shell

// components/ScanField.qml — the home input (UX §3). Submits on Enter/Return and re-emits
// edits so the view-model owns the text. Engine-agnostic: it only raises `submit`.
TextField {
    id: field
    signal submit()

    placeholderText: qsTr("Scan or type item code…  (F12 to return here)")
    font.family: Theme.monoFamily
    font.pixelSize: Theme.fontLarge
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectByMouse: true
    padding: Theme.pad

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radius
        border.color: field.activeFocus ? Theme.accent : Theme.border
        border.width: field.activeFocus ? 2 : 1
    }

    Keys.onReturnPressed: field.submit()
    Keys.onEnterPressed: field.submit()
}
