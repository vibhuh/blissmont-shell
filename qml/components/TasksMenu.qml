import QtQuick
import QtQuick.Controls.Basic
import Blissmont.Shell

// components/TasksMenu.qml — the Tasks launcher (brief §5). A lightweight popup menu (NOT a
// modal, NOT a workspace): pick an item → the menu closes → the host opens that workflow's
// right-side panel/screen or runs the immediate action. "Menus choose, panels work." Flat list
// for now (sections come when it grows past ~15 items). Each item raises `selected(action)`;
// the host (BillingScreen.doAction) maps the action to a nav/command. Payout is gated on
// payout_enabled, exactly as its Ctrl+O shortcut is.
Menu {
    id: menu
    signal selected(string action)

    // Whether a shift is currently open — gates Begin Day (open a shift only when none is open).
    property bool shiftOpen: false

    // Themed surface for the popup.
    background: Rectangle {
        implicitWidth: 220
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radius
    }

    // One uniform, themed entry. Touch/keyboard activatable; closes the menu on click
    // (Menu does that), then raises the chosen action to the host.
    component TaskItem: MenuItem {
        id: mi
        // NB: not "action" — AbstractButton.action is FINAL and cannot be overridden.
        property string taskAction: ""
        height: visible ? Theme.actionButton : 0
        contentItem: Text {
            text: mi.text
            color: mi.enabled ? Theme.text : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            verticalAlignment: Text.AlignVCenter
            leftPadding: Theme.unit
        }
        background: Rectangle {
            color: mi.highlighted ? Theme.surfaceAlt : "transparent"
            radius: Theme.radiusSmall
        }
        onTriggered: menu.selected(mi.taskAction)
    }

    TaskItem { text: qsTr("History");     taskAction: "history" }
    TaskItem { text: qsTr("Payout");      taskAction: "payout"; enabled: ConfigService.payoutEnabled }
    TaskItem { text: qsTr("Cash In");     taskAction: "cashin" }
    MenuSeparator {}
    TaskItem { text: qsTr("Open Drawer"); taskAction: "opendrawer" }
    TaskItem { text: qsTr("X Report");    taskAction: "xreport" }
    // Shift / day lifecycle (UX §12). Begin Day opens a shift (only when none is open);
    // Close Shift ends the open one; Z Report closes the store's day.
    // In SINGLE shift-management mode there is no explicit shift lifecycle — the register is opened
    // implicitly (auto Open-Register prompt) and closed at EOD — so these two items are hidden
    // (register-session slice, contracts v1.10.0). Z Report (the day close) stays in every mode.
    TaskItem { text: qsTr("Begin Day");   taskAction: "beginday";   enabled: !menu.shiftOpen
               visible: ConfigService.shiftManagementMode !== "single" }
    TaskItem { text: qsTr("Close Shift"); taskAction: "closeshift"; enabled: menu.shiftOpen
               visible: ConfigService.shiftManagementMode !== "single" }
    TaskItem { text: qsTr("Z Report");    taskAction: "zreport" }
    MenuSeparator {}
    TaskItem { text: qsTr("Calculator");  taskAction: "calculator" }
    TaskItem { text: qsTr("Settings");    taskAction: "settings" }
}
