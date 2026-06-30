import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/CalculatorPanel.qml — a small LOCAL calculator (Tasks launcher). Local only: it never
// touches the engine, the cart, or money — it's a cashier convenience (totting up change, a
// quick sum). A simple two-operand state machine (accumulator + pending op), so there is no
// eval() and no float-formatting surprises beyond JS arithmetic. Keyboard-driven as well as
// tap: digits, + - * /, Enter/=, Esc/C, Backspace.
Item {
    id: panel
    signal done()

    // Calculator state: the displayed entry, the stored accumulator, the pending operator,
    // and whether the next digit starts a fresh entry (after an operator or equals).
    property string display: "0"
    property real accumulator: 0
    property string pendingOp: ""
    property bool freshEntry: true

    function inputDigit(d) {
        if (panel.freshEntry) { panel.display = (d === "." ? "0." : d); panel.freshEntry = false; return }
        if (d === "." && panel.display.indexOf(".") !== -1) return
        panel.display = (panel.display === "0" && d !== ".") ? d : panel.display + d
    }
    function applyPending() {
        var cur = parseFloat(panel.display)
        if (panel.pendingOp === "") { panel.accumulator = cur; return }
        switch (panel.pendingOp) {
        case "+": panel.accumulator += cur; break
        case "-": panel.accumulator -= cur; break
        case "*": panel.accumulator *= cur; break
        case "/": panel.accumulator = (cur === 0 ? NaN : panel.accumulator / cur); break
        }
    }
    function setOp(op) {
        panel.applyPending()
        panel.pendingOp = op
        panel.display = panel.fmt(panel.accumulator)
        panel.freshEntry = true
    }
    function equals() {
        panel.applyPending()
        panel.pendingOp = ""
        panel.display = panel.fmt(panel.accumulator)
        panel.freshEntry = true
    }
    function clearAll() {
        panel.display = "0"; panel.accumulator = 0; panel.pendingOp = ""; panel.freshEntry = true
    }
    function backspace() {
        if (panel.freshEntry) return
        panel.display = panel.display.length > 1 ? panel.display.slice(0, -1) : "0"
        if (panel.display === "") panel.display = "0"
    }
    // Trim a float to at most 2 decimals without trailing-zero noise; guard NaN/Infinity.
    function fmt(v) {
        if (!isFinite(v)) return "Error"
        return parseFloat(v.toFixed(2)).toString()
    }

    focus: true
    Keys.onPressed: (event) => {
        var t = event.text
        if (t >= "0" && t <= "9") { panel.inputDigit(t); event.accepted = true }
        else if (t === ".") { panel.inputDigit("."); event.accepted = true }
        else if (t === "+" || t === "-" || t === "*" || t === "/") { panel.setOp(t); event.accepted = true }
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || t === "=") { panel.equals(); event.accepted = true }
        else if (event.key === Qt.Key_Backspace) { panel.backspace(); event.accepted = true }
        else if (event.key === Qt.Key_Escape) { panel.clearAll(); event.accepted = true }
    }

    component CalcButton: Button {
        id: cb
        property bool accent: false
        Layout.fillWidth: true
        Layout.fillHeight: true
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontLarge
        contentItem: Text {
            text: cb.text
            color: Theme.text
            font: cb.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radius
            color: cb.down ? Theme.surfaceAlt : (cb.accent ? Theme.surfaceAlt : Theme.surface)
            border.color: cb.accent ? Theme.accent : Theme.border
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Calculator")
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontLarge
            }
            Button {
                text: qsTr("Close")
                onClicked: panel.done()
            }
        }

        // ── Display ───────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: Theme.actionButton
            radius: Theme.radius
            color: Theme.bg
            border.color: Theme.border
            Text {
                anchors.fill: parent
                anchors.margins: Theme.unit
                text: panel.display
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontTotal
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideLeft
            }
        }

        // ── Keypad ──────────────────────────────────────────────────────────────
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 4
            rowSpacing: Theme.unit
            columnSpacing: Theme.unit

            CalcButton { text: "C"; accent: true; onClicked: panel.clearAll() }
            CalcButton { text: "←"; accent: true; onClicked: panel.backspace() }
            CalcButton { text: "÷"; accent: true; onClicked: panel.setOp("/") }
            CalcButton { text: "×"; accent: true; onClicked: panel.setOp("*") }

            CalcButton { text: "7"; onClicked: panel.inputDigit("7") }
            CalcButton { text: "8"; onClicked: panel.inputDigit("8") }
            CalcButton { text: "9"; onClicked: panel.inputDigit("9") }
            CalcButton { text: "−"; accent: true; onClicked: panel.setOp("-") }

            CalcButton { text: "4"; onClicked: panel.inputDigit("4") }
            CalcButton { text: "5"; onClicked: panel.inputDigit("5") }
            CalcButton { text: "6"; onClicked: panel.inputDigit("6") }
            CalcButton { text: "+"; accent: true; onClicked: panel.setOp("+") }

            CalcButton { text: "1"; onClicked: panel.inputDigit("1") }
            CalcButton { text: "2"; onClicked: panel.inputDigit("2") }
            CalcButton { text: "3"; onClicked: panel.inputDigit("3") }
            CalcButton { text: "="; accent: true; Layout.rowSpan: 2; onClicked: panel.equals() }

            CalcButton { text: "0"; Layout.columnSpan: 2; onClicked: panel.inputDigit("0") }
            CalcButton { text: "."; onClicked: panel.inputDigit(".") }
        }
    }
}
