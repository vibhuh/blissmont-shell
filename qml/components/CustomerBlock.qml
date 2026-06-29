import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/CustomerBlock.qml — zone 2, bottom-left (spec customer block). Defaults to
// "Walk-in customer"; Select opens customer search in the RIGHT panel (the host swaps
// nav state on requestSelect). Shows Points / Price level / bill-level Disc. Points and
// price level are not yet projected over the terminal contract, so they read muted
// placeholders; the customer label and bill discount are live from the cart summary.
Rectangle {
    id: block
    signal requestSelect()

    readonly property var s: PosEngineBridge.summary
    readonly property bool isWalkIn: s.customerLabel === "" || s.customerLabel === qsTr("Walk-in")

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.unit

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: block.isWalkIn ? qsTr("Walk-in customer")
                                         : block.s.customerLabel
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("Mobile-first search · create new")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                }
            }
            Button {
                id: selectBtn
                text: block.isWalkIn ? qsTr("Select") : qsTr("Change")
                onClicked: block.requestSelect()
                contentItem: Text {
                    text: selectBtn.text
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: selectBtn.down ? Theme.surfaceAlt : Theme.bg
                    radius: Theme.radius
                    border.color: selectBtn.hovered ? Theme.accent : Theme.border
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // Points · Price level · bill-level Disc.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            Column {
                Text { text: qsTr("Points"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                Text { text: "—"; color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }
            }
            Column {
                Text { text: qsTr("Price level"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                Text { text: "—"; color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
            }
            Item { Layout.fillWidth: true }
            Column {
                Text { text: qsTr("Disc"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignRight; width: parent.width }
                Text {
                    text: Format.money(block.s.orderDiscount)
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }
}
