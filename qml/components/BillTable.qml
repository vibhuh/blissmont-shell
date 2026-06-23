import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/BillTable.qml — the bill lines. Binds to the engine's CartLineModel via role
// names; renders verbatim (no client-side math). Header + a row delegate per line.
Rectangle {
    id: table
    property alias model: list.model

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.unit
        spacing: 0

        // Column header.
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            spacing: Theme.gap
            Text { text: qsTr("#"); color: Theme.textMuted; Layout.preferredWidth: 28; font.family: Theme.fontFamily }
            Text { text: qsTr("Item"); color: Theme.textMuted; Layout.fillWidth: true; font.family: Theme.fontFamily }
            Text { text: qsTr("Qty"); color: Theme.textMuted; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily }
            Text { text: qsTr("Price"); color: Theme.textMuted; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily }
            Text { text: qsTr("Total"); color: Theme.textMuted; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: RowLayout {
                width: ListView.view.width
                height: 36
                spacing: Theme.gap
                required property int lineNo
                required property string description
                required property string sku
                required property string qty
                required property string unitPrice
                required property string lineTotal

                Text { text: lineNo; color: Theme.textMuted; Layout.preferredWidth: 28; font.family: Theme.monoFamily }
                Text { text: description.length ? description : sku; color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight; font.family: Theme.fontFamily }
                Text { text: qty; color: Theme.text; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; font.family: Theme.monoFamily }
                Text { text: unitPrice; color: Theme.text; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; font.family: Theme.monoFamily }
                Text { text: lineTotal; color: Theme.text; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; font.family: Theme.monoFamily }
            }

            // Empty state — the skeleton boots to this until the first scan lands.
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("Scan an item to begin")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }
    }
}
