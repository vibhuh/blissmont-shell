import QtQuick
import QtQuick.Controls.Basic
import Blissmont.Shell

// components/ThinScrollBar.qml — the ONE vertical scrollbar for the shell's scrolling
// surfaces (bill grid, product list/grid, and the other panel lists). Attach it as
// `ScrollBar.vertical: ThinScrollBar {}` on any ListView/GridView/Flickable.
//
// Why not a bare Controls.Basic ScrollBar: its default handle sits at opacity 0 and only
// fades in while the handle itself is hovered/pressed or the view is actively flicked —
// so a keyboard- or wheel-scrolled list (our lookup lists) shows NO indicator at rest,
// and the cashier never learns the list scrolls. This handle is always visible whenever
// the content overflows (size < 1.0), themed for light/dark, and only brightens on
// hover/drag. AsNeeded still hides it entirely when everything fits.
ScrollBar {
    id: sb
    policy: ScrollBar.AsNeeded
    // Slightly inset so the handle doesn't kiss the panel border.
    rightPadding: 2
    contentItem: Rectangle {
        implicitWidth: 6
        radius: width / 2
        color: Theme.textMuted
        // Visible whenever the view overflows; brighter while grabbed/hovered.
        visible: sb.size < 1.0 && sb.policy !== ScrollBar.AlwaysOff
        opacity: sb.pressed ? 0.9 : (sb.hovered ? 0.7 : 0.45)
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }
}
