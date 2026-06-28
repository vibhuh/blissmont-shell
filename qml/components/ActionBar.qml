import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/ActionBar.qml — zone 4 (spec): bill-scoped, icon-only, UNIFORM action bar.
// Eight equal-size buttons, no appended amounts (Charge is the same size as the rest).
// Each carries a tooltip (hover desktop + long-press touch) and a keyboard shortcut hint;
// enable/disable is driven by cart state. The bar does NOT mutate cart state — it raises
// `triggered(name)` and the host (BillingScreen) maps the name to a nav/command, so the
// same handler backs both these buttons and Main.qml's global F-key shortcuts.
Rectangle {
    id: bar
    // State the gating reads (host-bound). cartActive == cart has lines (status "active").
    property bool cartActive: false
    property bool allowReturns: true
    signal triggered(string name)

    implicitHeight: Theme.actionButton + 2 * Theme.unit
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    // One uniform action button — icon glyph + tooltip + shortcut hint. Icon-only: the
    // label/shortcut live in the tooltip, never as appended text (keeps all eight uniform).
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

        ActionButton { glyph: "\u{1F4BE}"; label: qsTr("Save");    shortcutHint: "F2";  enabled: bar.cartActive; onClicked: bar.triggered("save") }
        ActionButton { glyph: "\u{1F5A8}"; label: qsTr("Print");   shortcutHint: "";    enabled: bar.cartActive; onClicked: bar.triggered("print") }
        ActionButton { glyph: "⏸";    label: qsTr("Hold");    shortcutHint: "F7";  enabled: bar.cartActive; onClicked: bar.triggered("hold") }
        ActionButton { glyph: "↩";    label: qsTr("Return");  shortcutHint: "F9";  enabled: bar.allowReturns; onClicked: bar.triggered("return") }
        ActionButton { glyph: "\u{1F551}"; label: qsTr("History"); shortcutHint: "F11"; enabled: true; onClicked: bar.triggered("history") }
        ActionButton { glyph: "…";    label: qsTr("Misc");    shortcutHint: "F6";  enabled: bar.cartActive; onClicked: bar.triggered("misc") }
        ActionButton { glyph: "✕";    label: qsTr("Clear");   shortcutHint: "F3";  danger: true; enabled: bar.cartActive; onClicked: bar.triggered("clear") }
        ActionButton { glyph: "₹";    label: qsTr("Charge");  shortcutHint: "F12"; enabled: bar.cartActive; onClicked: bar.triggered("charge") }
    }
}
