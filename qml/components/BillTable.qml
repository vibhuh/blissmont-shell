import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/BillTable.qml — zone 2, top (spec bill grid). Binds to the engine's
// CartLineModel by role name and renders verbatim (no client-side math). The Item cell
// carries a name + "HSN · GST%" subline. The SELECTED row expands inline to its
// LINE-scoped actions (qty − n + / Discount / Price override / Comment / Void) — these
// are distinct from the bill-scoped bottom action bar. A discounted line surfaces its
// discount in the subline.
//
// Columns are COLUMN-CONFIGURABLE per the old app's Sale Table Selection (which columns,
// width, align, visible). The terminal contract carries SaleColumn, but ConfigService
// does not project it yet, so this ships the sensible defaults (Item / Qty / Rate /
// Amount); wire the widths/visibility to ConfigService.saleColumns when it lands.
Rectangle {
    id: table
    property alias model: list.model

    // Column widths (defaults; future: drive from ConfigService.saleColumns).
    readonly property int wNo: 28
    readonly property int wQty: 56
    readonly property int wRate: 90
    readonly property int wAmount: 100

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    clip: true

    function stepQty(cur, delta) {
        var n = parseFloat(cur)
        if (isNaN(n)) n = 0
        n += delta
        if (n < 0) n = 0
        // Trim a trailing ".0" so integer steps stay integer-looking.
        return (n % 1 === 0) ? String(n) : String(n)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.unit
        spacing: 0

        // ── Column header ─────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            spacing: Theme.gap
            Text { text: qsTr("#"); color: Theme.textMuted; Layout.preferredWidth: table.wNo; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Item"); color: Theme.textMuted; Layout.fillWidth: true; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Qty"); color: Theme.textMuted; Layout.preferredWidth: table.wQty; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Rate"); color: Theme.textMuted; Layout.preferredWidth: table.wRate; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Amount"); color: Theme.textMuted; Layout.preferredWidth: table.wAmount; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: -1

            delegate: Column {
                id: row
                width: ListView.view ? ListView.view.width : 0
                required property int index
                required property int lineNo
                required property string description
                required property string sku
                required property string hsn
                required property string qty
                required property string unitPrice
                required property bool priceOverridden
                required property string discount
                required property string taxRate
                required property string lineTotal

                readonly property bool expanded: ListView.isCurrentItem
                readonly property bool discounted: discount !== "" && discount !== "0.00" && discount !== "0"

                // ── The line row ──────────────────────────────────────────────
                Rectangle {
                    width: row.width
                    height: 44
                    color: row.expanded ? Theme.surfaceAlt : "transparent"
                    radius: Theme.radiusSmall

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 0
                        anchors.rightMargin: 0
                        spacing: Theme.gap

                        Text {
                            text: row.lineNo
                            color: Theme.textMuted
                            Layout.preferredWidth: table.wNo
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Item: name + "HSN · GST%" subline (and discount note if any).
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Text {
                                text: row.description.length ? row.description : row.sku
                                color: Theme.text
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                            }
                            Text {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                color: row.discounted ? Theme.warn : Theme.textMuted
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSmall
                                text: {
                                    var parts = []
                                    if (row.hsn !== "") parts.push(row.hsn)
                                    // taxRate arrives as a FRACTION (e.g. "0.05"); show it as a
                                    // percent ("5%"). Round to drop float noise / trailing zeros.
                                    if (row.taxRate !== "") {
                                        var pct = parseFloat(row.taxRate) * 100
                                        if (!isNaN(pct)) parts.push((Math.round(pct * 100) / 100) + "%")
                                    }
                                    var base = parts.join(" · ")
                                    if (row.discounted)
                                        base = (base ? base + "   " : "") + qsTr("less %1").arg(row.discount)
                                    return base
                                }
                            }
                        }

                        Text {
                            text: row.qty
                            color: Theme.text
                            Layout.preferredWidth: table.wQty
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: row.unitPrice
                            color: row.priceOverridden ? Theme.accent : Theme.text
                            Layout.preferredWidth: table.wRate
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: row.lineTotal
                            color: Theme.text
                            Layout.preferredWidth: table.wAmount
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: list.currentIndex = (row.expanded ? -1 : row.index)
                    }
                }

                // ── Expanded line-scoped actions (spec: qty − n + / Discount /
                //    Price override / Comment / Void) ───────────────────────────
                Loader {
                    width: row.width
                    active: row.expanded
                    visible: active
                    sourceComponent: lineActions
                }

                Component {
                    id: lineActions
                    Rectangle {
                        width: row.width
                        implicitHeight: actionsCol.implicitHeight + 2 * Theme.unit
                        color: Theme.surfaceAlt
                        radius: Theme.radiusSmall

                        ColumnLayout {
                            id: actionsCol
                            anchors.fill: parent
                            anchors.margins: Theme.unit
                            spacing: Theme.unit

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.gap

                                // Qty stepper: − n +
                                RowLayout {
                                    spacing: Theme.unit
                                    Button {
                                        text: "−"; implicitWidth: 36
                                        onClicked: PosEngineBridge.setQty(row.lineNo, table.stepQty(row.qty, -1))
                                    }
                                    Text {
                                        text: row.qty; color: Theme.text
                                        font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                        horizontalAlignment: Text.AlignHCenter
                                        Layout.minimumWidth: 40
                                    }
                                    Button {
                                        text: "+"; implicitWidth: 36
                                        onClicked: PosEngineBridge.setQty(row.lineNo, table.stepQty(row.qty, 1))
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: qsTr("Comment")
                                    // No comment command on the terminal contract yet — present
                                    // per spec, wired when the command lands.
                                    enabled: false
                                }
                                Button {
                                    id: voidBtn
                                    text: qsTr("Void")
                                    onClicked: { PosEngineBridge.removeLine(row.lineNo); list.currentIndex = -1 }
                                    contentItem: Text {
                                        text: voidBtn.text; color: Theme.danger
                                        font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        color: voidBtn.down ? Theme.surface : "transparent"
                                        radius: Theme.radius; border.color: Theme.danger
                                    }
                                }
                            }

                            // Discount + price override inline inputs.
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.gap

                                Text { text: qsTr("Disc"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                                TextField {
                                    id: discField
                                    Layout.preferredWidth: 90
                                    text: row.discounted ? row.discount : ""
                                    placeholderText: qsTr("0.00")
                                    font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                    color: Theme.text; placeholderTextColor: Theme.textMuted
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    onAccepted: PosEngineBridge.setLineDiscount(row.lineNo, text)
                                }
                                Button {
                                    text: qsTr("Apply")
                                    onClicked: PosEngineBridge.setLineDiscount(row.lineNo, discField.text)
                                }

                                Item { Layout.preferredWidth: Theme.gap }

                                Text { text: qsTr("Price"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                                TextField {
                                    id: priceField
                                    Layout.preferredWidth: 90
                                    text: ""
                                    placeholderText: row.unitPrice
                                    font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                    color: Theme.text; placeholderTextColor: Theme.textMuted
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    onAccepted: if (text !== "") PosEngineBridge.setPriceOverride(row.lineNo, text)
                                }
                                Button {
                                    text: qsTr("Override")
                                    enabled: priceField.text !== ""
                                    onClicked: PosEngineBridge.setPriceOverride(row.lineNo, priceField.text)
                                }
                            }
                        }
                    }
                }
            }

            // Empty state — boots here until the first scan/quick-key lands.
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
