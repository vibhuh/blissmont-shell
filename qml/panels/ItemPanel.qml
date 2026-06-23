import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// panels/ItemPanel.qml — the persistent right-panel placeholder (spec §6).
// In the skeleton it shows the active context label; post-skeleton each context
// (item / tender / payout / history / return) becomes its own panel swapped in here.
Rectangle {
    id: panel
    property string context: "item"

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Panel: %1").arg(panel.context)
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }
        Text {
            text: qsTr("Placeholder — tender, payout, history and return panels swap in here.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true }
    }
}
