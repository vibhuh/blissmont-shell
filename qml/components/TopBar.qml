import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/TopBar.qml — zone 1 (spec layout): store · counter · bill# on the left,
// online/offline + shift state on the right. Identity fields (store/counter/bill#/shift)
// are not yet projected over the terminal contract, so they bind to properties the host
// sets from what IS available and fall back to clearly-muted placeholders; the connection
// indicator is live (ConnectionService). When store identity / shift state land on the
// contract, point these properties at them — the layout does not change.
Rectangle {
    id: bar
    property string storeName: ""
    property string counterId: ""
    property string billNo: ""
    property string shiftState: ""
    property bool online: false
    property string connectionText: ""

    implicitHeight: 44
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pad
        anchors.rightMargin: Theme.pad
        spacing: Theme.gap

        // ── Identity (left) ───────────────────────────────────────────────────
        Text {
            text: bar.storeName !== "" ? bar.storeName : qsTr("Store")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.bold: true
        }
        Text { text: "·"; color: Theme.border; font.pixelSize: Theme.fontBody }
        Text {
            text: bar.counterId !== "" ? qsTr("Counter %1").arg(bar.counterId) : qsTr("Counter —")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        Text { text: "·"; color: Theme.border; font.pixelSize: Theme.fontBody }
        Text {
            text: bar.billNo !== "" ? qsTr("Bill %1").arg(bar.billNo) : qsTr("New bill")
            color: Theme.textMuted
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontBody
        }

        Item { Layout.fillWidth: true }

        // ── Theme toggle (live light/dark switch, no restart) ─────────────────
        AbstractButton {
            id: themeBtn
            implicitWidth: Theme.iconButton
            implicitHeight: Theme.iconButton
            hoverEnabled: true
            onClicked: Theme.toggle()
            contentItem: Text {
                text: Theme.isDark ? "☀" : "☾"   // sun → go light; moon → go dark
                color: Theme.text
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: Theme.radius
                color: themeBtn.hovered ? Theme.surfaceAlt : "transparent"
                border.color: themeBtn.hovered ? Theme.accent : Theme.border
            }
            ToolTip.text: Theme.isDark ? qsTr("Switch to light theme")
                                       : qsTr("Switch to dark theme")
            ToolTip.visible: hovered
            ToolTip.delay: 300
        }
        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 18; color: Theme.border }

        // ── Shift state + connection (right) ──────────────────────────────────
        Text {
            text: bar.shiftState !== "" ? bar.shiftState : qsTr("No shift")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 18; color: Theme.border }
        Rectangle {
            width: 10; height: 10; radius: 5
            color: bar.online ? Theme.ok : Theme.danger
        }
        Text {
            text: bar.connectionText !== "" ? bar.connectionText
                                            : (bar.online ? qsTr("Online") : qsTr("Offline"))
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
    }
}
