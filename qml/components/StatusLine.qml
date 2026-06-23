import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// components/StatusLine.qml — inline transient feedback + connection indicator (UX §2).
// No popups for status; this single line carries item-not-found, rejections, settle results.
Rectangle {
    id: bar
    property string message: ""
    property string connectionText: ""
    property bool online: false

    implicitHeight: 32
    color: Theme.surfaceAlt
    radius: Theme.radius

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pad
        anchors.rightMargin: Theme.pad
        spacing: Theme.gap

        Rectangle {
            width: 10; height: 10; radius: 5
            color: bar.online ? Theme.ok : Theme.danger
        }
        Text {
            text: bar.connectionText
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: Theme.border }
        Text {
            Layout.fillWidth: true
            text: bar.message
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
        }
    }
}
