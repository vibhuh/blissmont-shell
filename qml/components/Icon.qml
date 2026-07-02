import QtQuick
import QtQuick.Shapes
import Blissmont.Shell

// components/Icon.qml — the ONE icon family (Phase 3). Minimal, modern, enterprise-grade
// line icons: stroke-based on a 24×24 grid, uniform 2px stroke, rounded caps/joins, themed
// colour. Every icon in the shell comes from here so stroke weight and size stay consistent
// (no more mixed unicode glyphs ₹/⏸/☰ that each font renders differently). Set `name`,
// `color`, `size`; an unknown name renders nothing.
Item {
    id: root
    property string name: ""
    property color color: Theme.text
    property real size: Theme.iconMd
    property real stroke: 2

    implicitWidth: size
    implicitHeight: size

    // Path data authored on a 24×24 grid; scaled to `size` by the wrapper below.
    readonly property var _paths: ({
        "charge":   "M3 6 H21 V18 H3 Z M3 10 H21 M6 15 H11",                       // card / payment
        "hold":     "M9 5 V19 M15 5 V19",                                          // pause
        "discount": "M7 17 L17 7 M6.5 8 a1.6 1.6 0 1 0 3.2 0 a1.6 1.6 0 1 0 -3.2 0 M14.3 16 a1.6 1.6 0 1 0 3.2 0 a1.6 1.6 0 1 0 -3.2 0",  // percent
        "sundry":   "M12 5 V19 M5 12 H19",                                         // plus
        "print":    "M7 9 V4 H17 V9 M5 9 H19 V16 H17 V20 H7 V16 H5 Z M7 14 H17 V20 H7 Z M16 12 h0.01",  // printer
        "void":     "M5 7 H19 M9 7 V5 a1 1 0 0 1 1 -1 H14 a1 1 0 0 1 1 1 V7 M6.5 7 L7.5 19.5 a1 1 0 0 0 1 0.9 H15.5 a1 1 0 0 0 1 -0.9 L18.5 7 M10 11 V17 M14 11 V17",  // trash
        "tasks":    "M4 7 H20 M4 12 H20 M4 17 H20",                                // menu
        "search":   "M4 10.5 a6 6 0 1 0 12 0 a6 6 0 1 0 -12 0 M14.5 14.5 L20 20",  // magnifier
        "customer": "M8.5 8 a3.5 3.5 0 1 0 7 0 a3.5 3.5 0 1 0 -7 0 M5 20 a7 5 0 0 1 14 0",  // user
        "plus":     "M12 6 V18 M6 12 H18",
        "minus":    "M6 12 H18",
        "check":    "M5 12 L10 17 L19 6",
        "close":    "M6 6 L18 18 M18 6 L6 18",
        "chevronDown":  "M6 9 L12 15 L18 9",
        "chevronLeft":  "M15 6 L9 12 L15 18",
        "price":    "M13 4 H20 V11 L11.5 19.5 a1.5 1.5 0 0 1 -2.1 0 L4.5 14.6 a1.5 1.5 0 0 1 0 -2.1 L13 4 Z M16.5 7.5 a1 1 0 1 0 0.01 0",  // tag
        "grid":     "M4 4 H10 V10 H4 Z M14 4 H20 V10 H14 Z M4 14 H10 V20 H4 Z M14 14 H20 V20 H14 Z"
    })

    Item {
        width: 24
        height: 24
        anchors.centerIn: parent
        scale: root.size / 24
        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: root.stroke
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathSvg { path: root._paths[root.name] !== undefined ? root._paths[root.name] : "" }
            }
        }
    }
}
