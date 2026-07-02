import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// components/ListRow.qml — the ONE list row (Tier 3.4), reused across every title/subtitle/value
// list (quick keys, product search, history, customer search). A single visual OBJECT, not
// independently-positioned text:
//   • a prominent title over a muted, smaller subtitle (SKU/tax/date), as a vertical stack;
//   • an optional right value (price/total) VERTICALLY CENTERED against the WHOLE block — never
//     pinned to the first line (the old quick-keys bug);
//   • consistent top/bottom padding so it reads like a card, not text on a strip;
//   • a full-row selection highlight, hover state, and click target.
// The caller formats the right value (Format.money/amount) so this stays presentation-only.
// Used as a ListView delegate directly: bind `selected` to ListView.isCurrentItem and handle
// `clicked` / `doubleClicked`.
Rectangle {
    id: rowRoot

    property string title: ""
    property string subtitle: ""
    property string rightValue: ""
    property color  rightValueColor: rowRoot.selected ? Theme.selectionText : Theme.text
    property bool   selected: false

    signal clicked()
    signal doubleClicked()

    // Card-like height: at least the standard row height, but grows to fit a two-line stack
    // with comfortable padding on both ends.
    implicitHeight: Math.max(Theme.rowHeight, textCol.implicitHeight + 2 * Theme.spaceMd)
    radius: Theme.radius
    // Full-row highlight: selected (keyboard) first, then hover (mouse); calm at rest.
    color: rowRoot.selected ? Theme.selectionBg
                            : (hover.hovered ? Theme.surfaceAlt : "transparent")

    HoverHandler { id: hover }
    TapHandler {
        onTapped: rowRoot.clicked()
        onDoubleTapped: rowRoot.doubleClicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        spacing: Theme.gap

        // Title + subtitle stack — vertically centered so the right value can center against it.
        ColumnLayout {
            id: textCol
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: rowRoot.title
                color: rowRoot.selected ? Theme.selectionText : Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: rowRoot.subtitle !== ""
                text: rowRoot.subtitle
                color: rowRoot.selected ? Theme.selectionText : Theme.textMuted
                opacity: rowRoot.selected ? 0.85 : 1.0
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
        }

        // Right value: mono, right-aligned, and centered against the WHOLE block (the fix).
        Text {
            visible: rowRoot.rightValue !== ""
            Layout.alignment: Qt.AlignVCenter
            text: rowRoot.rightValue
            color: rowRoot.rightValueColor
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontBody
            horizontalAlignment: Text.AlignRight
        }
    }
}
