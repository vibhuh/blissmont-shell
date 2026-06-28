import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/ProductSearchPanel.qml — the right panel's HOME state (spec): the scan/search
// field with an inline icon scope toggle, category chips, and quick-keys in either a
// LIST (keyboard/scanner-dense, supermarket) or GRID (≥64px touch tiles, limited-SKU)
// interaction model. The panel ALWAYS returns here after a takeover. The live scan path
// stays the engine's (Enter dispatches scanItem via the billing view-model); the
// quick-keys dispatch addLine.
//
// DEFERRED DATA: there is no product-search/catalog contract on the engine yet (the
// engine serves a fake catalog with no path to seeded data). So the chips/quick-keys
// bind to a small in-shell DEMO catalog below, and search filters it client-side. When a
// catalog/search contract lands, swap `catalog` for the engine-backed model — the search
// field, scope toggle, chips, and both quick-key interaction models stay as-is.
Item {
    id: panel
    property var vm: null                       // BillingViewModel (for the live scan path)
    property string scope: "all"                // "all" (sku+name+barcode) | "barcode"
    property string catalogMode: "list"         // "list" | "grid" (Configuration-defaulted by SKU count)
    property string activeCategory: "All"

    function focusSearch() { searchField.forceActiveFocus() }
    Component.onCompleted: searchField.forceActiveFocus()  // scan-is-home

    // ── In-shell DEMO catalog (placeholder for the deferred engine catalog) ──────
    readonly property var catalog: [
        { id: "1001", name: "Toor Dal 1kg",      sku: "TD1",  barcode: "8901001", price: "120.00", category: "Grocery", hsn: "0713", gst: "5" },
        { id: "1002", name: "Basmati Rice 5kg",  sku: "BR5",  barcode: "8901002", price: "560.00", category: "Grocery", hsn: "1006", gst: "5" },
        { id: "2001", name: "Full Cream Milk 1L",sku: "FCM1", barcode: "8902001", price: "62.00",  category: "Dairy",   hsn: "0401", gst: "0" },
        { id: "2002", name: "Paneer 200g",       sku: "PN2",  barcode: "8902002", price: "95.00",  category: "Dairy",   hsn: "0406", gst: "5" },
        { id: "3001", name: "Hand Wash 250ml",   sku: "HW2",  barcode: "8903001", price: "99.00",  category: "Care",    hsn: "3401", gst: "18" },
        { id: "3002", name: "Shampoo 180ml",     sku: "SH1",  barcode: "8903002", price: "165.00", category: "Care",    hsn: "3305", gst: "18" }
    ]

    readonly property var categories: {
        var seen = {}, out = ["All"]
        for (var i = 0; i < catalog.length; ++i)
            if (!seen[catalog[i].category]) { seen[catalog[i].category] = true; out.push(catalog[i].category) }
        return out
    }

    readonly property var filtered: {
        var q = searchField.text.toLowerCase().trim()
        return catalog.filter(function (p) {
            if (panel.activeCategory !== "All" && p.category !== panel.activeCategory) return false
            if (q === "") return true
            if (panel.scope === "barcode") return p.barcode.toLowerCase().indexOf(q) >= 0
            return (p.name + " " + p.sku + " " + p.barcode).toLowerCase().indexOf(q) >= 0
        })
    }

    function submitSearch() {
        var t = searchField.text.trim()
        if (t === "") return
        if (panel.vm) { panel.vm.scanText = t; panel.vm.submitScan() }
        else PosEngineBridge.scanItem(t)
        searchField.text = ""
    }
    function pick(p) {
        PosEngineBridge.addLine(p.id, "1")
        searchField.text = ""
        searchField.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.unit

        // ── Search field + inline scope toggle (icon-only, tooltip) ──────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: panel.scope === "barcode" ? qsTr("Scan barcode…")
                                                           : qsTr("Scan or search item…")
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontBody
                color: Theme.text
                placeholderTextColor: Theme.textMuted
                padding: Theme.unit
                background: Rectangle {
                    color: Theme.bg; radius: Theme.radius
                    border.color: searchField.activeFocus ? Theme.accent : Theme.border
                    border.width: searchField.activeFocus ? 2 : 1
                }
                Keys.onReturnPressed: panel.submitSearch()
                Keys.onEnterPressed: panel.submitSearch()
            }

            // Scope toggle: list-search icon = "all" (sku+name+barcode); barcode icon =
            // "barcode-only" (single indexed column — fast at 50k–300k SKU).
            AbstractButton {
                id: scopeBtn
                implicitWidth: Theme.iconButton + Theme.unit
                implicitHeight: Theme.iconButton + Theme.unit
                hoverEnabled: true
                onClicked: panel.scope = (panel.scope === "all" ? "barcode" : "all")
                contentItem: Text {
                    text: panel.scope === "barcode" ? "|||" : "\u{1F50E}"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radius; color: scopeBtn.down ? Theme.surfaceAlt : Theme.bg
                    border.color: scopeBtn.hovered ? Theme.accent : Theme.border
                }
                ToolTip.text: panel.scope === "barcode"
                              ? qsTr("Barcode-only search (fast). Tap for all fields.")
                              : qsTr("Search all: SKU · name · barcode. Tap for barcode-only.")
                ToolTip.visible: hovered || scopeLong.pressed
                ToolTip.delay: 300
                TapHandler { id: scopeLong; acceptedDevices: PointerDevice.TouchScreen; longPressThreshold: 0.5 }
            }
        }

        // ── Category chips (one mechanism; data per vertical) ────────────────────
        Flow {
            Layout.fillWidth: true
            spacing: Theme.unit
            Repeater {
                model: panel.categories
                delegate: AbstractButton {
                    id: chip
                    required property string modelData
                    implicitHeight: Theme.chipHeight
                    leftPadding: Theme.gap; rightPadding: Theme.gap
                    hoverEnabled: true
                    readonly property bool active: panel.activeCategory === modelData
                    onClicked: panel.activeCategory = modelData
                    contentItem: Text {
                        text: chip.modelData
                        color: chip.active ? Theme.bg : Theme.text
                        font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: Theme.chipHeight / 2
                        color: chip.active ? Theme.accent : Theme.surfaceAlt
                        border.color: chip.active ? Theme.accent : Theme.border
                    }
                }
            }
        }

        // ── Quick-keys header: list/grid toggle (icon-only) ──────────────────────
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Quick keys")
                color: Theme.textMuted
                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            }
            Item { Layout.fillWidth: true }
            AbstractButton {
                id: modeBtn
                implicitWidth: Theme.iconButton; implicitHeight: Theme.iconButton
                hoverEnabled: true
                onClicked: panel.catalogMode = (panel.catalogMode === "list" ? "grid" : "list")
                contentItem: Text {
                    text: panel.catalogMode === "list" ? "▦" : "≣"
                    color: Theme.text; font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { radius: Theme.radius; color: modeBtn.hovered ? Theme.surfaceAlt : "transparent"; border.color: Theme.border }
                ToolTip.text: panel.catalogMode === "list" ? qsTr("Switch to grid (touch tiles)")
                                                           : qsTr("Switch to list (keyboard density)")
                ToolTip.visible: hovered
                ToolTip.delay: 300
            }
        }

        // ── Quick-keys: LIST model (dense, keyboard/scanner) ─────────────────────
        ListView {
            id: listView
            visible: panel.catalogMode === "list"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.filtered
            spacing: 1
            delegate: ItemDelegate {
                id: lrow
                required property var modelData
                width: ListView.view ? ListView.view.width : 0
                height: 40
                onClicked: panel.pick(lrow.modelData)
                contentItem: RowLayout {
                    spacing: Theme.gap
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text { text: lrow.modelData.name; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: lrow.modelData.sku + " · " + lrow.modelData.hsn + " · " + lrow.modelData.gst + "%"; color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                    }
                    Text { text: lrow.modelData.price; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }
                }
                background: Rectangle { color: lrow.hovered ? Theme.surfaceAlt : "transparent"; radius: Theme.radiusSmall }
            }
        }

        // ── Quick-keys: GRID model (≥64px touch tiles) ───────────────────────────
        GridView {
            id: gridView
            visible: panel.catalogMode === "grid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.filtered
            cellWidth: Math.max(Theme.touchMin + Theme.gap, width / Math.max(1, Math.floor(width / (Theme.touchMin + Theme.gap))))
            cellHeight: Theme.touchMin + Theme.gap
            delegate: Item {
                id: tile
                required property var modelData
                width: gridView.cellWidth
                height: gridView.cellHeight
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: Theme.unit / 2
                    radius: Theme.radius
                    color: tileTap.pressed ? Theme.surfaceAlt : Theme.bg
                    border.color: Theme.border
                    // Tiles meet the ≥64px touch-target minimum (spec).
                    implicitWidth: Theme.touchMin
                    implicitHeight: Theme.touchMin
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.unit
                        spacing: 2
                        Text { text: tile.modelData.name; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; Layout.fillWidth: true; Layout.fillHeight: true }
                        Text { text: tile.modelData.price; color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontSmall }
                    }
                    TapHandler { id: tileTap; onTapped: panel.pick(tile.modelData) }
                }
            }
        }

        // Deferred-data note (kept honest while the catalog is the in-shell demo).
        Text {
            Layout.fillWidth: true
            visible: panel.filtered.length === 0
            text: qsTr("No matches in the demo catalog. (Live catalog/search pending engine contract.)")
            color: Theme.textMuted
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
