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

    // Config freshness (contracts v1.18.0). Empty configWarning = nothing to say;
    // the badge takes no space at all in the healthy case, so the normal status line
    // is unchanged and the warning is genuinely unusual when it appears.
    property string configWarning: ""

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

        // ── Config-staleness badge ────────────────────────────────────────────
        // Right-aligned and visible only when the engine says the config is stale.
        // Warn, not danger: the till is fully operational — it is trading on config
        // that has stopped being refreshed, which is a "get this looked at" and not
        // a "stop selling". Colouring it danger would push staff to halt a shop that
        // does not need halting, and staff who learn to ignore a red badge are worse
        // off than staff who never had one.
        Rectangle {
            visible: bar.configWarning !== ""
            Layout.preferredWidth: warnRow.implicitWidth + 2 * Theme.pad
            Layout.preferredHeight: 22
            radius: Theme.radius
            color: Qt.rgba(Theme.warn.r, Theme.warn.g, Theme.warn.b, 0.15)
            border.width: 1
            border.color: Theme.warn

            RowLayout {
                id: warnRow
                anchors.centerIn: parent
                spacing: 6
                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: Theme.warn
                }
                Text {
                    text: bar.configWarning
                    color: Theme.warn
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }
}
