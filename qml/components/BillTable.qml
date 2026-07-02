import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/BillTable.qml — zone 2, top (spec bill grid). Binds to the engine's
// CartLineModel by role name and renders verbatim (no client-side math). The Item cell
// carries a name + "HSN · GST%" subline. Each row carries a compact trash (Void) in its
// last column (Phase 3 — Void left the expanded editor). The SELECTED row expands inline
// to its compact line-scoped editor: qty −[ ]+ / Discount field + apply / Price field +
// apply. A discounted line surfaces its discount in the subline.
//
// Columns are COLUMN-CONFIGURABLE per the old app's Sale Table Selection (which columns,
// width, align, visible). The terminal contract carries SaleColumn, but ConfigService
// does not project it yet, so this ships the sensible defaults (Item / Qty / Rate /
// Amount); wire the widths/visibility to ConfigService.saleColumns when it lands.
Rectangle {
    id: table
    property alias model: list.model

    // Column widths (defaults; future: drive from ConfigService.saleColumns).
    readonly property int wNo: 24
    readonly property int wQty: 56
    readonly property int wRate: 92
    readonly property int wAmount: 104
    readonly property int wVoid: 32

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    clip: true

    function stepQty(cur, delta) {
        var n = parseFloat(cur)
        if (isNaN(n)) n = 0
        n += delta
        if (n < 0) n = 0
        return String(n)
    }

    // A compact, low-chrome icon button used by the row trash and the inline editor.
    component MiniIconButton: AbstractButton {
        id: mib
        property string iconName: ""
        property bool danger: false
        implicitWidth: 30
        implicitHeight: 30
        hoverEnabled: true
        contentItem: Item {
            Icon {
                anchors.centerIn: parent
                name: mib.iconName
                size: Theme.iconSm
                color: !mib.enabled ? Theme.textMuted
                       : (mib.danger ? Theme.danger : Theme.text)
            }
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: mib.down ? Theme.surfaceAlt : "transparent"
            border.width: 1
            border.color: mib.hovered && mib.enabled
                          ? (mib.danger ? Theme.danger : Theme.accent)
                          : Theme.border
            opacity: mib.enabled ? 1.0 : 0.5
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.unit
        spacing: 0

        // ── Column header ─────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.leftMargin: Theme.unit
            Layout.rightMargin: Theme.unit
            spacing: Theme.gap
            Text { text: qsTr("#"); color: Theme.textMuted; Layout.preferredWidth: table.wNo; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Item"); color: Theme.textMuted; Layout.fillWidth: true; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Qty"); color: Theme.textMuted; Layout.preferredWidth: table.wQty; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Rate"); color: Theme.textMuted; Layout.preferredWidth: table.wRate; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Text { text: qsTr("Amount"); color: Theme.textMuted; Layout.preferredWidth: table.wAmount; horizontalAlignment: Text.AlignRight; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
            Item { Layout.preferredWidth: table.wVoid }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spaceXs
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
                    id: lineRect
                    width: row.width
                    height: Theme.rowHeight
                    color: row.expanded ? Theme.surfaceAlt : "transparent"
                    radius: Theme.radiusSmall

                    HoverHandler { id: rowHover }

                    // Accent selection marker (restrained palette — accent on selected row).
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3; height: parent.height - Theme.unit
                        radius: 1.5
                        color: Theme.accent
                        visible: row.expanded
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.unit
                        anchors.rightMargin: Theme.unit
                        spacing: Theme.gap

                        Text {
                            text: row.lineNo
                            color: Theme.textMuted
                            Layout.preferredWidth: table.wNo
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontSmall
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
                                visible: text !== ""
                                text: {
                                    var parts = []
                                    if (row.hsn !== "") parts.push(row.hsn)
                                    // taxRate arrives as a FRACTION (e.g. "0.05"); the one
                                    // formatter renders it as a percent ("5.00%").
                                    if (row.taxRate !== "") parts.push(Format.taxRate(row.taxRate))
                                    var base = parts.join(" · ")
                                    if (row.discounted)
                                        base = (base ? base + "   " : "") + qsTr("less %1").arg(Format.money(row.discount))
                                    return base
                                }
                            }
                        }

                        Text {
                            text: Format.qty(row.qty)
                            color: Theme.text
                            Layout.preferredWidth: table.wQty
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: Format.money(row.unitPrice)
                            color: row.priceOverridden ? Theme.accent : Theme.text
                            Layout.preferredWidth: table.wRate
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: Format.money(row.lineTotal)
                            color: Theme.text
                            Layout.preferredWidth: table.wAmount
                            horizontalAlignment: Text.AlignRight
                            font.family: Theme.monoFamily
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Void — last column, compact trash, revealed on hover/selection.
                        MiniIconButton {
                            Layout.preferredWidth: table.wVoid
                            Layout.alignment: Qt.AlignVCenter
                            iconName: "void"
                            danger: true
                            opacity: (rowHover.hovered || row.expanded) ? 1.0 : 0.0
                            onClicked: { PosEngineBridge.removeLine(row.lineNo); list.currentIndex = -1 }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        // Leave the trash column to its own button.
                        anchors.rightMargin: table.wVoid + Theme.unit
                        onClicked: list.currentIndex = (row.expanded ? -1 : row.index)
                    }
                }

                // ── Expanded line-scoped editor (qty / discount / price) — compact ──
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

                        RowLayout {
                            id: actionsCol
                            anchors.fill: parent
                            anchors.leftMargin: Theme.unit + table.wNo
                            anchors.rightMargin: Theme.unit
                            anchors.topMargin: Theme.unit
                            anchors.bottomMargin: Theme.unit
                            spacing: Theme.gap

                            // Qty: − [editable textbox] + — decimal entry for weighed goods
                            // (1.250 kg, 0.750 L), numeric-keypad friendly (Phase 2).
                            RowLayout {
                                spacing: Theme.spaceXs
                                Text { text: qsTr("Qty"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                                MiniIconButton {
                                    iconName: "minus"
                                    onClicked: PosEngineBridge.setQty(row.lineNo, table.stepQty(row.qty, -1))
                                }
                                TextField {
                                    id: qtyField
                                    text: Format.qty(row.qty)
                                    Layout.preferredWidth: 60
                                    horizontalAlignment: Text.AlignHCenter
                                    font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                    color: Theme.text
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    selectByMouse: true
                                    onAccepted: qtyField.commitQty()
                                    onEditingFinished: qtyField.commitQty()
                                    function commitQty() {
                                        var v = text.trim()
                                        if (v !== "" && v !== Format.qty(row.qty))
                                            PosEngineBridge.setQty(row.lineNo, v)
                                    }
                                    background: Rectangle {
                                        color: Theme.surface
                                        radius: Theme.radiusSmall
                                        border.color: qtyField.activeFocus ? Theme.accent : Theme.border
                                    }
                                }
                                MiniIconButton {
                                    iconName: "plus"
                                    onClicked: PosEngineBridge.setQty(row.lineNo, table.stepQty(row.qty, 1))
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Discount: field + compact apply icon (no big "Apply" button).
                            Text { text: qsTr("Disc"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                            TextField {
                                id: discField
                                Layout.preferredWidth: 76
                                text: row.discounted ? Format.plain(row.discount) : ""
                                placeholderText: qsTr("0.00")
                                horizontalAlignment: Text.AlignRight
                                font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                color: Theme.text; placeholderTextColor: Theme.textMuted
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onAccepted: PosEngineBridge.setLineDiscount(row.lineNo, text)
                                background: Rectangle { color: Theme.surface; radius: Theme.radiusSmall; border.color: discField.activeFocus ? Theme.accent : Theme.border }
                            }
                            MiniIconButton {
                                iconName: "check"
                                onClicked: PosEngineBridge.setLineDiscount(row.lineNo, discField.text)
                            }

                            // Price override: field + compact apply icon.
                            Text { text: qsTr("Price"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                            TextField {
                                id: priceField
                                Layout.preferredWidth: 76
                                text: ""
                                placeholderText: Format.plain(row.unitPrice)
                                horizontalAlignment: Text.AlignRight
                                font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                                color: Theme.text; placeholderTextColor: Theme.textMuted
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onAccepted: if (text !== "") PosEngineBridge.setPriceOverride(row.lineNo, text)
                                background: Rectangle { color: Theme.surface; radius: Theme.radiusSmall; border.color: priceField.activeFocus ? Theme.accent : Theme.border }
                            }
                            MiniIconButton {
                                iconName: "check"
                                enabled: priceField.text !== ""
                                onClicked: PosEngineBridge.setPriceOverride(row.lineNo, priceField.text)
                            }
                        }
                    }
                }
            }

            // Empty state — a confident "ready" treatment (Tier 3.5): the terminal is set,
            // just waiting for the first item rather than nagging the cashier.
            ColumnLayout {
                anchors.centerIn: parent
                visible: list.count === 0
                spacing: Theme.spaceXs
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Ready")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontMedium
                    font.bold: true
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Scan a barcode or search to add an item")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }
}
