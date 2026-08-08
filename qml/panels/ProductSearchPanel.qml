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
// ENGINE-BACKED CATALOG: the catalogue is the device's real synced product cache, pulled via
// PosEngineBridge.listProducts() (engine replies ProductList → productsListed) and ranked
// client-side by the SAME controller. Works fully offline. A row PICK dispatches
// AddLine(item_id) exactly as before; the field, scope toggle, chips, quick-key views, and all
// keyboard behaviour are unchanged — only the data source moved from an in-shell demo array to
// the engine.
Item {
    id: panel
    property var vm: null                       // BillingViewModel (for the live scan path)
    property string scope: "all"                // "all" (sku+name+barcode) | "barcode"
    property string catalogMode: "list"         // "list" | "grid" (Configuration-defaulted by SKU count)
    property string activeCategory: "All"

    function focusSearch() { searchField.forceActiveFocus() }
    Component.onCompleted: {
        PosEngineBridge.listProducts()               // pull the device's real catalogue
        lookup.setItems(panel.itemsForCategory)
        searchField.forceActiveFocus()               // scan-is-home
    }

    // ── Engine-backed catalog ────────────────────────────────────────────────────
    // The device's synced product cache (PosEngineBridge.products, a QVariantList of
    // {id,name,sku,barcode,price,hsn,gst,category}). Bound to the NOTIFY property so a fresh
    // ProductList reply re-drives the reactive chain below (categories → itemsForCategory →
    // lookup.setItems). A row PICK sends AddLine(id) — the engine resolves the line.
    readonly property var catalog: PosEngineBridge.products

    readonly property var categories: {
        var seen = {}, out = ["All"]
        for (var i = 0; i < catalog.length; ++i) {
            var c = catalog[i].category
            if (c && c !== "All" && !seen[c]) { seen[c] = true; out.push(c) }
        }
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
                // A failed scan reports ON this field (Tier 3.3), not the status bar.
                readonly property bool hasError: panel.vm && panel.vm.scanError !== ""
                placeholderText: panel.scope === "barcode" ? qsTr("Scan barcode…")
                                                           : qsTr("Scan or search item…")
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontBody
                color: Theme.text
                placeholderTextColor: Theme.textMuted
                padding: Theme.unit
                background: Rectangle {
                    color: Theme.bg; radius: Theme.radius
                    // Error border wins; otherwise accent on focus, neutral at rest.
                    border.color: searchField.hasError ? Theme.danger
                                  : (searchField.activeFocus ? Theme.accent : Theme.border)
                    border.width: (searchField.hasError || searchField.activeFocus) ? 2 : 1
                }
                // The field is the source of typed text; push it into the ranked controller.
                // Also mirror it into the view-model so the status bar can read a "Scanning…"
                // state and any stale not-found hint clears the instant the cashier edits.
                onTextChanged: {
                    lookup.searchText = text
                    if (panel.vm) panel.vm.scanText = text
                }
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

        // Inline scan error (Tier 3.3): the "not found" hint lives at the field, in danger
        // colour, and disappears the moment the cashier types again.
        Text {
            Layout.fillWidth: true
            visible: panel.vm && panel.vm.scanError !== ""
            text: panel.vm ? panel.vm.scanError : ""
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
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
            // A long result set must scroll — a themed, always-on-overflow handle.
            ScrollBar.vertical: ThinScrollBar {}
            model: lookup                              // the ranked + filtered controller
            currentIndex: lookup.currentIndex          // highlight is list-state, bound to the controller
            spacing: 1
            keyNavigationEnabled: false                // navigation is routed via LookupKeys, not Qt defaults
            // Keep the highlighted row on-screen when the controller moves it (arrows/page/refilter).
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            // Type-to-refine + arrow/Enter/Tab while the table has focus all route through the reference impl.
            Keys.onPressed: (event) => keys.handleTableKey(event)
            // The ONE list row (components/ListRow.qml): name over SKU·HSN·GST%, price centered
            // against the whole block, full-row highlight = current (keyboard) first then hover.
            delegate: ListRow {
                id: lrow
                required property var item              // the "item" role = the catalog payload map
                required property int index
                width: ListView.view ? ListView.view.width : 0
                selected: ListView.isCurrentItem
                title: lrow.item.name
                subtitle: lrow.item.sku + " · " + lrow.item.hsn + " · " + lrow.item.gst + "%"
                rightValue: Format.money(lrow.item.price)
                onClicked: { lookup.setCurrentIndex(lrow.index); panel.pick(lrow.item) }
            }
        }

        // ── Quick-keys: GRID model (touch tiles) — same controller, touch-first ──
        // Touch tiles carry ART (R2.4): a per-item image (item.image) or a line icon from
        // the one Icon family (item.icon); when the catalog gives neither, a name monogram
        // so every tile still reads as a deliberate button. The engine-backed catalog can
        // populate image/icon later with no layout change.
        GridView {
            id: gridView
            visible: panel.catalogMode === "grid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            // A long tile set must scroll — a themed, always-on-overflow handle.
            ScrollBar.vertical: ThinScrollBar {}
            model: lookup
            // Roomier tile than the bare 64px min so art + name + price breathe, still well
            // above the ≥64px touch-target minimum. Square-ish, packed to fill the width.
            readonly property int minTile: Theme.touchMin + Theme.iconLg + Theme.unit
            cellWidth: Math.max(minTile, width / Math.max(1, Math.floor(width / minTile)))
            cellHeight: gridView.cellWidth + Theme.gap
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
                    implicitWidth: Theme.touchMin
                    implicitHeight: Theme.touchMin
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.unit
                        spacing: Theme.unit / 2
                        // ── Tile art (R2.4): image › icon › monogram, in priority order ──
                        Item {
                            id: art
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: Theme.iconLg
                            readonly property string src:   tile.item.image !== undefined ? tile.item.image : ""
                            readonly property string glyph: tile.item.icon  !== undefined ? tile.item.icon  : ""
                            // Per-item image (engine-backed catalog): cropped square, rounded.
                            Image {
                                anchors.centerIn: parent
                                width: Math.min(parent.width, parent.height)
                                height: width
                                visible: art.src !== "" && status === Image.Ready
                                source: art.src
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                                clip: true
                            }
                            // Line icon from the one family (Icon.qml).
                            Icon {
                                anchors.centerIn: parent
                                visible: art.src === "" && art.glyph !== ""
                                name: art.glyph
                                size: Theme.iconLg
                                color: Theme.textMuted
                            }
                            // Fallback: first-letter monogram so a bare item still reads as a tile.
                            Rectangle {
                                anchors.centerIn: parent
                                visible: art.src === "" && art.glyph === ""
                                width: Theme.iconLg + Theme.unit
                                height: width
                                radius: Theme.radiusSmall
                                color: Theme.surfaceAlt
                                Text {
                                    anchors.centerIn: parent
                                    text: (tile.item.name && tile.item.name.length > 0) ? tile.item.name.charAt(0).toUpperCase() : "?"
                                    color: Theme.textMuted
                                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; font.bold: true
                                }
                            }
                        }
                        Text { text: tile.item.name; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                        Text { text: Format.money(tile.item.price); color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
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
