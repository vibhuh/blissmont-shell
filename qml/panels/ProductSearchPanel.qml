import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/ProductSearchPanel.qml — the right panel's HOME state (spec): the scan/search
// field with an inline icon scope toggle, category chips, and quick-keys in either a
// LIST (keyboard/scanner-dense, supermarket) or GRID (≥64px touch tiles, limited-SKU)
// interaction model. The panel ALWAYS returns here after a takeover. The live scan path
// stays the engine's (Enter with no catalog match dispatches scanItem via the billing
// view-model); selecting a result dispatches addLine.
//
// KEYBOARD-FIRST LOOKUP (SHELL_KEYBOARD_LOOKUP brief, Part 1) — and the REFERENCE every
// future lookup (customer, supplier, ledger, warehouse, tax-code) reuses. The ranked filter
// + highlight state live in C++ (LookupController, unit-tested); the Qt key-routing lives in
// the reusable LookupKeys.qml. This panel is the FIRST CONSUMER and the template: it owns only
// its own layout (scope toggle, category chips, list/grid views) and binds them to that one
// controller + one key-router. To add a lookup elsewhere, instantiate the same two and lay out
// to taste.
//
// DEFERRED DATA: there is no product-search/catalog contract on the engine yet (the engine
// serves a fake catalog with no path to seeded data). So the chips/quick-keys bind to a small
// in-shell DEMO catalog below, and the lookup ranks it client-side. When a catalog/search
// contract lands, feed the engine-backed rows into the SAME controller — the field, scope
// toggle, chips, both quick-key views, and all keyboard behavior stay as-is.
Item {
    id: panel
    property var vm: null                       // BillingViewModel (for the live scan path)
    property string scope: "all"                // "all" (sku+name+barcode) | "barcode"
    property string catalogMode: "list"         // "list" | "grid" (Configuration-defaulted by SKU count)
    property string activeCategory: "All"

    function focusSearch() { searchField.forceActiveFocus() }
    Component.onCompleted: { lookup.setItems(panel.itemsForCategory); searchField.forceActiveFocus() }  // scan-is-home

    // ── In-shell DEMO catalog (placeholder for the deferred engine catalog) ──────
    // ALIGNED to the dev engine's seeded products (rachis-core cmd/blissmont-engine
    // demoProducts) so a row PICK dispatches addLine() with an item id the engine can
    // resolve (it looks up AddLine by item_id) — i.e. clicking a row actually lands a
    // line in the cart against the dev engine. The `id` here MUST equal the engine's
    // ItemId. When a real catalog/search contract lands, this whole list is replaced by
    // the engine-backed model (see header note) and the alignment becomes moot.
    readonly property var catalog: [
        { id: "ITEM-A",    name: "Widget",          sku: "SKU-A",   barcode: "111",     price: "100.00", category: "Hardware", hsn: "—", gst: "18" },
        { id: "ITEM-B",    name: "Gadget",          sku: "SKU-B",   barcode: "222",     price: "250.00", category: "Hardware", hsn: "—", gst: "18" },
        { id: "ITEM-TEST", name: "Smoke Test Item", sku: "TESTSKU", barcode: "TESTSKU", price: "100.00", category: "Test",     hsn: "—", gst: "18" }
    ]

    readonly property var categories: {
        var seen = {}, out = ["All"]
        for (var i = 0; i < catalog.length; ++i)
            if (!seen[catalog[i].category]) { seen[catalog[i].category] = true; out.push(catalog[i].category) }
        return out
    }

    // The category pre-filter feeds the controller; the controller does the ranked text search.
    readonly property var itemsForCategory: {
        if (panel.activeCategory === "All") return panel.catalog
        return panel.catalog.filter(function (p) { return p.category === panel.activeCategory })
    }
    onItemsForCategoryChanged: lookup.setItems(panel.itemsForCategory)

    // Live scan path: code not in the catalog goes to the engine (preserves scanner behavior).
    function submitSearch() {
        var t = searchField.text.trim()
        if (t === "") return
        if (panel.vm) { panel.vm.scanText = t; panel.vm.submitScan() }
        else PosEngineBridge.scanItem(t)
        searchField.text = ""
    }
    function pick(p) {
        if (!p || !p.id) return
        PosEngineBridge.addLine(p.id, "1")
        searchField.text = ""
        searchField.forceActiveFocus()
    }

    // ── The ranked-lookup engine (C++) + reusable key-routing ────────────────────
    LookupController {
        id: lookup
        searchKeys: ["name", "sku", "barcode"]   // "all" scope; name is primary (drives word-starts)
        barcodeKey: "barcode"                     // "barcode" scope
        scope: panel.scope
    }
    LookupKeys {
        id: keys
        controller: lookup
        field: searchField
        list: listView
        onPicked: (item) => panel.pick(item)
        onSubmitted: (text) => panel.submitSearch()   // no catalog match → live engine scan
        onAdvance: searchField.forceActiveFocus()      // Items has no next field; stay home
        // onEscaped: panel is already home — nothing to close.
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
                // The field is the source of typed text; push it into the ranked controller.
                onTextChanged: lookup.searchText = text
                // All field key behavior (Enter/Tab/Down/Esc) routes through the reference impl.
                Keys.onPressed: (event) => keys.handleFieldKey(event)
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

        // ── Quick-keys: LIST model (dense, keyboard/scanner) — the keyboard-first view ──
        ListView {
            id: listView
            visible: panel.catalogMode === "list"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: lookup                              // the ranked + filtered controller
            currentIndex: lookup.currentIndex          // highlight is list-state, bound to the controller
            spacing: 1
            keyNavigationEnabled: false                // navigation is routed via LookupKeys, not Qt defaults
            // Keep the highlighted row on-screen when the controller moves it (arrows/page/refilter).
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            // Type-to-refine + arrow/Enter/Tab while the table has focus all route through the reference impl.
            Keys.onPressed: (event) => keys.handleTableKey(event)
            delegate: ItemDelegate {
                id: lrow
                required property var item              // the "item" role = the catalog payload map
                required property int index
                width: ListView.view ? ListView.view.width : 0
                height: 40
                readonly property bool current: ListView.isCurrentItem
                onClicked: { lookup.setCurrentIndex(index); panel.pick(lrow.item) }
                contentItem: RowLayout {
                    spacing: Theme.gap
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text { text: lrow.item.name; color: lrow.current ? Theme.selectionText : Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: lrow.item.sku + " · " + lrow.item.hsn + " · " + lrow.item.gst + "%"; color: lrow.current ? Theme.selectionText : Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall }
                    }
                    Text { text: lrow.item.price; color: lrow.current ? Theme.selectionText : Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }
                }
                // Highlight = current row (keyboard) first, then hover (mouse). Visible in both themes.
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: lrow.current ? Theme.selectionBg : (lrow.hovered ? Theme.surfaceAlt : "transparent")
                }
            }
        }

        // ── Quick-keys: GRID model (≥64px touch tiles) — same controller, touch-first ──
        GridView {
            id: gridView
            visible: panel.catalogMode === "grid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: lookup
            cellWidth: Math.max(Theme.touchMin + Theme.gap, width / Math.max(1, Math.floor(width / (Theme.touchMin + Theme.gap))))
            cellHeight: Theme.touchMin + Theme.gap
            delegate: Item {
                id: tile
                required property var item
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
                        Text { text: tile.item.name; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; Layout.fillWidth: true; Layout.fillHeight: true }
                        Text { text: tile.item.price; color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontSmall }
                    }
                    TapHandler { id: tileTap; onTapped: panel.pick(tile.item) }
                }
            }
        }

        // Deferred-data note (kept honest while the catalog is the in-shell demo).
        Text {
            Layout.fillWidth: true
            visible: lookup.count === 0
            text: qsTr("No matches in the demo catalog. (Live catalog/search pending engine contract.)")
            color: Theme.textMuted
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
