import QtQuick

// components/LookupKeys.qml — the reusable keyboard ROUTING for every BlissMont lookup
// (SHELL_KEYBOARD_LOOKUP brief, Part 1). Non-visual: it owns the Qt-specific key traps the
// brief calls out, so no lookup re-implements them and none inherits the bug. The ranking +
// highlight state lives in C++ (LookupController); THIS just wires keys to it.
//
// Wire it from a lookup's TextField and results list:
//   LookupController { id: controller; searchKeys: [...]; barcodeKey: "barcode" }
//   LookupKeys { id: keys; controller: controller; field: searchField; list: resultsView
//                onPicked: (item) => ...; onSubmitted: (t) => ...; onEscaped: ...; onAdvance: ... }
//   TextField { id: searchField; onTextChanged: controller.searchText = text
//               Keys.onPressed: (e) => keys.handleFieldKey(e) }
//   ListView  { id: resultsView; model: controller; currentIndex: controller.currentIndex
//               Keys.onPressed: (e) => keys.handleTableKey(e) }
//
// The three key-classes are routed deliberately (the brief's mode-less model):
//   • navigation (↑ ↓ PgUp PgDn Home End) — move the current row, focus stays in the table;
//   • selection  (Enter, Tab)             — select the current row (Tab also advances);
//   • printable  (letters/digits/…)       — from the table, jump back to the field and append,
//                                           so the cashier refines without reaching for the mouse.
QtObject {
    id: keys

    property var controller            // LookupController (model + ranked state)
    property Item field                // the search TextField (source of typed text)
    property var list                  // the results ListView/TableView
    property int pageRows: 8           // rows a PageUp/PageDown jumps

    // A row was selected (Enter / Tab on a resolvable selection).
    signal picked(var item)
    // Enter/Tab with NO resolvable selection — live-scan fallback (engine scan path).
    signal submitted(string text)
    // Esc on an already-empty field — the caller closes the lookup/panel.
    signal escaped()
    // Tab — the caller advances to the next field (Tab === Enter + advance).
    signal advance()

    // Select the highlighted/exact/single row per LookupController's priority. Returns whether
    // a row was actually picked (false → caller may fall back to submitted()).
    function selectCurrent() {
        if (!controller) return false
        var it = controller.selectedItem()
        if (it && Object.keys(it).length > 0) { keys.picked(it); return true }
        return false
    }

    function _enterOrSubmit() {
        if (!selectCurrent())
            keys.submitted(field ? field.text.trim() : "")
    }

    // Keys while focus is in the SEARCH FIELD.
    function handleFieldKey(event) {
        switch (event.key) {
        case Qt.Key_Return:
        case Qt.Key_Enter:
            event.accepted = true
            _enterOrSubmit()
            return
        case Qt.Key_Tab:
        case Qt.Key_Backtab:
            // Tab === Enter (NOT default focus-advance): select + advance. Must consume the event.
            event.accepted = true
            _enterOrSubmit()
            keys.advance()
            return
        case Qt.Key_Down:
            // Move focus INTO the table; the highlighted row stays current (no move).
            event.accepted = true
            if (controller && controller.count > 0 && list) list.forceActiveFocus()
            return
        case Qt.Key_Up:
            event.accepted = true
            if (controller && controller.count > 0 && list) list.forceActiveFocus()
            return
        case Qt.Key_PageDown:
            event.accepted = true
            if (controller && controller.count > 0) { if (list) list.forceActiveFocus(); controller.movePage(keys.pageRows) }
            return
        case Qt.Key_PageUp:
            event.accepted = true
            if (controller && controller.count > 0) { if (list) list.forceActiveFocus(); controller.movePage(-keys.pageRows) }
            return
        case Qt.Key_Escape:
            event.accepted = true
            if (field && field.text.length > 0) field.text = ""   // clear text first
            else keys.escaped()                                   // already empty → close
            return
        default:
            event.accepted = false   // printable + editing keys stay with the TextField
        }
    }

    // Keys while focus is in the RESULTS TABLE.
    function handleTableKey(event) {
        if (!controller) { event.accepted = false; return }
        switch (event.key) {
        case Qt.Key_Up:       event.accepted = true; controller.moveUp(); return
        case Qt.Key_Down:     event.accepted = true; controller.moveDown(); return
        case Qt.Key_PageUp:   event.accepted = true; controller.movePage(-keys.pageRows); return
        case Qt.Key_PageDown: event.accepted = true; controller.movePage(keys.pageRows); return
        case Qt.Key_Home:     event.accepted = true; controller.moveHome(); return
        case Qt.Key_End:      event.accepted = true; controller.moveEnd(); return
        case Qt.Key_Return:
        case Qt.Key_Enter:
            event.accepted = true; _enterOrSubmit(); return
        case Qt.Key_Tab:
        case Qt.Key_Backtab:
            event.accepted = true; _enterOrSubmit(); keys.advance(); return
        case Qt.Key_Escape:
            // From the table, Esc returns focus to the field (does NOT close).
            event.accepted = true
            if (field) field.forceActiveFocus()
            return
        default:
            // Printable → jump to the field, append, let the filter re-run (re-highlights best).
            var t = event.text
            if (t && t.length === 1 && t.charCodeAt(0) >= 0x20) {
                event.accepted = true
                if (field) {
                    field.forceActiveFocus()
                    field.text = field.text + t      // → field.onTextChanged → controller re-filters
                    field.cursorPosition = field.text.length
                }
            } else {
                event.accepted = false               // non-printable control keys ignored here
            }
        }
    }
}
