import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// workflows/BeginDayWorkflow.qml — the Begin-Day (open shift) FULL-SCREEN workflow (UX §12).
//
// Opens the day's shift with an opening cash float — a distinct fiscal operation, full-screen
// (never a modal or right-panel takeover). The ENGINE owns the open (OpenShift →
// ShiftStateChanged(open), or CommandRejected(SHIFT_ALREADY_OPEN) when one is already open);
// this screen collects the float, dispatches, and reflects the outcome. Keyboard-first: Enter
// opens (or the float field's own Enter), Esc cancels.
Item {
    id: root
    signal closed()

    // Device-default cashier until the shell has a login (see PosEngineBridge::openShift).
    readonly property string cashierId: "cashier-1"

    // entry → opening → done | error
    property string phase: "entry"
    property string errorMessage: ""
    property string openedFloat: ""

    focus: true
    Component.onCompleted: floatField.forceActiveFocus()
    // Leaving the entry phase disables the float field; pull keyboard focus back to the root so
    // Enter/Esc keep driving the primary/cancel action in the opening/done/error phases.
    onPhaseChanged: if (root.phase !== "entry") root.forceActiveFocus()

    function submit() {
        var v = floatField.text.trim()
        if (v === "") v = "0"
        root.openedFloat = v
        root.phase = "opening"
        PosEngineBridge.openShift(root.cashierId, v)
    }
    function primary() {
        switch (root.phase) {
        case "entry":   root.submit(); break
        case "opening": break                 // waiting on the engine
        default:        root.closed()         // done / error → leave
        }
    }

    Keys.onEscapePressed: (e) => { root.closed(); e.accepted = true }
    // Enter drives the primary ONLY when the float field isn't the one handling it (done/error);
    // in the entry phase the focused field's own onAccepted submits (avoids a double dispatch).
    Keys.onReturnPressed: (e) => { if (root.phase !== "entry") { root.primary(); e.accepted = true } }
    Keys.onEnterPressed:  (e) => { if (root.phase !== "entry") { root.primary(); e.accepted = true } }

    Connections {
        target: PosEngineBridge
        function onShiftStateChanged(shiftId, status) {
            if (root.phase === "opening" && status === "open") root.phase = "done"
        }
        function onCommandRejected(code, message) {
            if (root.phase !== "opening") return
            root.errorMessage = message !== "" ? message : qsTr("Could not open the shift")
            root.phase = "error"
        }
    }

    // Opaque full-screen backdrop.
    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.55, 560)
        spacing: Theme.gap

        // ── Heading ──────────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: qsTr("Begin Day")
                color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontTotal; font.bold: true
            }
            Text {
                text: qsTr("Open the shift · opening cash float")
                color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // ── Body card (phase-driven) ─────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: body.implicitHeight + 2 * Theme.pad
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: root.phase === "done" ? Theme.ok
                        : root.phase === "error" ? Theme.danger
                        : Theme.border

            ColumnLayout {
                id: body
                anchors.fill: parent
                anchors.margins: Theme.pad
                spacing: Theme.gap

                // entry / opening — the opening-float field
                RowLayout {
                    visible: root.phase === "entry" || root.phase === "opening"
                    Layout.fillWidth: true
                    spacing: Theme.gap
                    Text {
                        text: qsTr("Opening float")
                        color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    }
                    Item { Layout.fillWidth: true }
                    TextField {
                        id: floatField
                        Layout.preferredWidth: 220
                        enabled: root.phase === "entry"
                        placeholderText: "0.00"
                        horizontalAlignment: Text.AlignRight
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 0; decimals: 2; notation: DoubleValidator.StandardNotation }
                        color: Theme.text; placeholderTextColor: Theme.textMuted
                        font.family: Theme.monoFamily; font.pixelSize: Theme.fontLarge
                        onAccepted: root.submit()
                        background: Rectangle {
                            radius: Theme.radiusSmall
                            color: Theme.surfaceAlt
                            border.color: floatField.activeFocus ? Theme.accent : Theme.border
                        }
                    }
                }
                Text {
                    visible: root.phase === "opening"
                    text: qsTr("Opening the shift…")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                }

                // done
                RowLayout {
                    visible: root.phase === "done"
                    Layout.fillWidth: true
                    spacing: Theme.unit
                    Text { text: "✔"; color: Theme.ok; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: qsTr("Shift open")
                            color: Theme.ok; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge; font.bold: true
                        }
                        Text {
                            text: qsTr("Opening float %1").arg(Format.money(root.openedFloat))
                            color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                        }
                    }
                }

                // error
                Text {
                    visible: root.phase === "error"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: root.errorMessage
                    color: Theme.danger; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                }
            }
        }

        // ── Actions ──────────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                id: cancelBtn
                visible: root.phase === "entry"
                Layout.preferredWidth: 160
                text: qsTr("Cancel  (Esc)")
                onClicked: root.closed()
                contentItem: Text {
                    text: cancelBtn.text; color: Theme.text
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitHeight: Theme.actionButton
                    color: Theme.surface; radius: Theme.radius; border.color: Theme.border
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: primaryBtn
                Layout.preferredWidth: 220
                enabled: root.phase !== "opening"
                text: root.phase === "entry" ? qsTr("Open Shift  (Enter)")
                    : root.phase === "opening" ? qsTr("Opening…")
                    : root.phase === "done" ? qsTr("Start Selling  (Enter)")
                    : qsTr("OK  (Enter)")
                onClicked: root.primary()
                contentItem: Text {
                    text: primaryBtn.text
                    color: primaryBtn.enabled ? Theme.selectionText : Theme.textMuted
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitHeight: Theme.actionButton
                    radius: Theme.radius
                    color: !primaryBtn.enabled ? Theme.surface
                         : root.phase === "error" ? Theme.danger
                         : root.phase === "done" ? Theme.ok
                         : Theme.accent
                    border.color: background.color
                }
            }
        }
    }
}
