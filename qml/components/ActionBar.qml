import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/ActionBar.qml — zone 4 (brief §4): the bill-scoped, icon-only, UNIFORM action bar.
// The final set is the seven business verbs of the current bill:
//   Charge (F12) · Hold · Discount · Sundry · Print · Void · Tasks ▾
// "Save" is gone (a software verb, ambiguous in a POS). Sale actions only live here; operational
// actions live under Tasks ▾ (the ▾ marks a menu, not an immediate action). Each button carries
// a tooltip (hover + long-press) and a shortcut hint; enable/disable is driven by cart state
// (and, for Print, by whether a just-settled bill exists to reprint). The bar does NOT mutate
// state — it raises `triggered(name)` and the host (BillingScreen) maps the name to a
// nav/command, so the same handler backs both these buttons and Main.qml's global F-keys.
Rectangle {
    id: bar
    // State the gating reads (host-bound). cartActive == cart has lines (status "active").
    property bool cartActive: false
    // canReprint == a bill was just settled this session, so Print can reprint it.
    property bool canReprint: false
    signal triggered(string name)

    // Open the Tasks launcher — called by the Tasks button and by the host's global shortcut.
    function openTasks() { tasksMenu.popup(tasksBtn, 0, -tasksMenu.height) }

    implicitHeight: Theme.actionButton + 2 * Theme.unit
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    // One uniform action button — icon glyph + tooltip + shortcut hint. Icon-only: the
    // label/shortcut live in the tooltip, never as appended text (keeps all seven uniform).
    component ActionButton: AbstractButton {
        id: btn
        property string glyph: ""
        property string label: ""
        property string shortcutHint: ""
        property bool danger: false
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.actionButton
        implicitHeight: Theme.actionButton
        hoverEnabled: true

        contentItem: Text {
            text: btn.glyph
            color: !btn.enabled ? Theme.textMuted
                                : (btn.danger ? Theme.danger : Theme.text)
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radius
            color: btn.down ? Theme.surfaceAlt : Theme.bg
            border.width: 1
            border.color: btn.danger && btn.enabled ? Theme.danger
                          : (btn.hovered && btn.enabled ? Theme.accent : Theme.border)
            opacity: btn.enabled ? 1.0 : 0.5
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

        ActionButton { glyph: "₹";    label: qsTr("Charge");   shortcutHint: "F12"; enabled: bar.cartActive; onClicked: bar.triggered("charge") }
        ActionButton { glyph: "⏸";    label: qsTr("Hold");     shortcutHint: "F7";  enabled: bar.cartActive; onClicked: bar.triggered("hold") }
        ActionButton { glyph: "%";    label: qsTr("Discount"); shortcutHint: "Ctrl+D"; enabled: bar.cartActive && ConfigService.allowDiscounts; onClicked: bar.triggered("discount") }
        ActionButton { glyph: "＋";    label: qsTr("Sundry");   shortcutHint: "F6";  enabled: bar.cartActive; onClicked: bar.triggered("sundry") }
        ActionButton { glyph: "\u{1F5A8}"; label: qsTr("Print"); shortcutHint: ""; enabled: bar.canReprint; onClicked: bar.triggered("print") }
        ActionButton { glyph: "✕";    label: qsTr("Void");     shortcutHint: "F3";  danger: true; enabled: bar.cartActive; onClicked: bar.triggered("clear") }
        ActionButton { id: tasksBtn; glyph: "☰"; label: qsTr("Tasks ▾"); shortcutHint: "F10"; enabled: true; onClicked: bar.openTasks() }
    }

    // The Tasks launcher popup (brief §5). Selecting an item closes the menu (Menu does that)
    // and re-raises the chosen action through the bar's own triggered(name), so the host maps
    // Tasks actions with the same handler as the bar buttons and F-keys.
    TasksMenu {
        id: tasksMenu
        onSelected: (action) => bar.triggered(action)
    }
}
