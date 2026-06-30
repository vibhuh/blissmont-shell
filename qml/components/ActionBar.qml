import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/ActionBar.qml — zone 4 (brief §4): the bill-scoped, icon+label, UNIFORM action bar.
// The final set is the seven business verbs of the current bill:
//   Charge (F12) · Hold · Discount · Sundry · Print · Void · Tasks ▾
// "Save" is gone (a software verb, ambiguous in a POS). Sale actions only live here; operational
// actions live under Tasks ▾ (the ▾ marks a menu, not an immediate action).
//
// Tier 1 refinement: each button is icon + a short TEXT LABEL beneath it (the constantly-used
// bar earns explicit labels over icon-only-with-tooltip). All seven keep an IDENTICAL footprint
// (equal width via fillWidth, equal height) with consistent icon size, spacing, and full
// hover/pressed/disabled states. The PRIMARY verb, Charge, is made unmistakable WITHOUT breaking
// that uniform footprint: it alone carries the accent fill and a bold "₹ Charge" label, so the
// cashier's eye lands on it at a glance (the squint test). Each button still carries a tooltip
// (hover + long-press); enable/disable is driven by cart state (and, for Print, by whether a
// just-settled bill exists to reprint). The bar does NOT mutate state — it raises
// `triggered(name)` and the host (BillingScreen) maps the name to a nav/command, so the same
// handler backs both these buttons and Main.qml's global F-keys.
Rectangle {
    id: bar
    // State the gating reads (host-bound). cartActive == cart has lines (status "active").
    property bool cartActive: false
    // canReprint == a bill was just settled this session, so Print can reprint it.
    property bool canReprint: false
    signal triggered(string name)

    // Open the Tasks launcher — called by the Tasks button and by the host's global shortcut.
    function openTasks() { tasksMenu.popup(tasksBtn, 0, -tasksMenu.height) }

    // Taller than the old icon-only bar to seat the icon + label stack comfortably.
    implicitHeight: Theme.actionButton + Theme.gap + 2 * Theme.unit
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    // One uniform action button — icon (from the single icon family) stacked over a short text
    // label. All seven stay identical in width AND height; only the colour/weight changes, never
    // the footprint. Restrained palette: only the PRIMARY verb (Charge) carries the accent fill +
    // a bold label; Void carries danger; the rest are neutral ghost buttons. Every button has
    // hover / pressed / disabled states.
    component ActionButton: AbstractButton {
        id: btn
        property string iconName: ""
        property string label: ""
        property string shortcutHint: ""
        property bool danger: false
        property bool primary: false
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.actionButton + Theme.gap
        implicitHeight: Theme.actionButton + Theme.gap
        hoverEnabled: true

        readonly property color fg: !enabled ? Theme.textMuted
                                    : primary ? Theme.selectionText
                                    : danger  ? Theme.danger
                                    : Theme.text

        contentItem: ColumnLayout {
            spacing: Theme.spaceXs
            Icon {
                Layout.alignment: Qt.AlignHCenter
                name: btn.iconName
                color: btn.fg
                size: Theme.iconMd
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: btn.label
                color: btn.fg
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                // The primary verb reads heavier — bold label on the accent fill.
                font.bold: btn.primary
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radius
            color: btn.primary
                   ? (btn.down ? Qt.darker(Theme.accent, 1.2)
                               : (btn.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent))
                   : (btn.down ? Theme.surfaceAlt
                               : (btn.hovered ? Theme.surfaceAlt : "transparent"))
            border.width: btn.primary ? 0 : 1
            // Neutral border at rest; on hover, danger (Void) or accent (the rest). Keeps
            // the bar calm — the red lives in the Void icon, not a persistent outline.
            border.color: !btn.enabled ? Theme.border
                          : (btn.hovered ? (btn.danger ? Theme.danger : Theme.accent)
                                         : Theme.border)
            opacity: btn.enabled ? 1.0 : 0.45
        }

        // Tooltip: hover (desktop) + long-press (touch). The full label + shortcut live here.
        ToolTip.text: btn.shortcutHint !== "" ? btn.label + "   (" + btn.shortcutHint + ")"
                                              : btn.label
        ToolTip.visible: hovered || longPress.pressed
        ToolTip.delay: 300
        TapHandler {
            id: longPress
            acceptedDevices: PointerDevice.TouchScreen
            longPressThreshold: 0.5
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.unit
        spacing: Theme.unit

        ActionButton { iconName: "charge";  primary: true; label: Format.currencySymbol + qsTr(" Charge"); shortcutHint: "F12"; enabled: bar.cartActive; onClicked: bar.triggered("charge") }
        ActionButton { iconName: "hold";    label: qsTr("Hold");     shortcutHint: "F7";  enabled: bar.cartActive; onClicked: bar.triggered("hold") }
        ActionButton { iconName: "discount"; label: qsTr("Discount"); shortcutHint: "Ctrl+D"; enabled: bar.cartActive && ConfigService.allowDiscounts; onClicked: bar.triggered("discount") }
        ActionButton { iconName: "sundry";  label: qsTr("Sundry");   shortcutHint: "F6";  enabled: bar.cartActive; onClicked: bar.triggered("sundry") }
        ActionButton { iconName: "print";   label: qsTr("Print");    shortcutHint: "";    enabled: bar.canReprint; onClicked: bar.triggered("print") }
        ActionButton { iconName: "void";    label: qsTr("Void");     shortcutHint: "F3";  danger: true; enabled: bar.cartActive; onClicked: bar.triggered("clear") }
        ActionButton { id: tasksBtn; iconName: "tasks"; label: qsTr("Tasks ▾"); shortcutHint: "F10"; enabled: true; onClicked: bar.openTasks() }
    }

    // The Tasks launcher popup (brief §5). Selecting an item closes the menu (Menu does that)
    // and re-raises the chosen action through the bar's own triggered(name), so the host maps
    // Tasks actions with the same handler as the bar buttons and F-keys.
    TasksMenu {
        id: tasksMenu
        onSelected: (action) => bar.triggered(action)
    }
}
