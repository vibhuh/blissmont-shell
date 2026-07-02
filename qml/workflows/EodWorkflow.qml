import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// workflows/EodWorkflow.qml — the End-of-Day (day close) FULL-SCREEN workflow (UX §12).
//
// EOD is a distinct fiscal operation from a sale: one per store per day, aggregating the
// day's closed shifts into the consolidated GL journal. Per the spec it is a full-screen
// workflow — never a modal or a right-panel takeover — because it is deliberate, attributable
// and lives at a different fiscal layer than billing. It owns the whole view while open.
//
// The ENGINE owns the close (RunEod → EodResult, or EodBlocked when a shift is still open);
// this screen only confirms, dispatches, and reflects the outcome. Keyboard-first: Enter
// drives the primary action of the current phase, Esc backs out.
Item {
    id: root

    // Raised when the workflow is finished (done/blocked acknowledged, or cancelled) so the
    // host can restore the sale screen and re-focus the scan field.
    signal closed()

    // confirm → running → done | blocked
    property string phase: "confirm"
    property string batchId: ""
    property bool   provisional: false
    property var    openShiftIds: []

    focus: true
    Component.onCompleted: root.forceActiveFocus()

    function primary() {
        switch (root.phase) {
        case "confirm": root.phase = "running"; PosEngineBridge.runEod(); break
        case "running": break                       // waiting on the engine — no-op
        default:        root.closed()               // done / blocked → leave
        }
    }

    Keys.onReturnPressed: (e) => { root.primary(); e.accepted = true }
    Keys.onEnterPressed:  (e) => { root.primary(); e.accepted = true }
    Keys.onEscapePressed: (e) => { root.closed();  e.accepted = true }

    Connections {
        target: PosEngineBridge
        function onEodResult(batchId, provisional) {
            if (root.phase !== "running") return
            root.batchId = batchId; root.provisional = provisional; root.phase = "done"
        }
        function onEodBlocked(openShiftIds) {
            if (root.phase !== "running") return
            root.openShiftIds = openShiftIds; root.phase = "blocked"
        }
    }

    // Opaque full-screen backdrop — this is a screen, not a translucent overlay.
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
                text: qsTr("End of Day")
                color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontTotal; font.bold: true
            }
            Text {
                text: qsTr("Z Report · store day close")
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
                        : root.phase === "blocked" ? Theme.warn
                        : Theme.border

            ColumnLayout {
                id: body
                anchors.fill: parent
                anchors.margins: Theme.pad
                spacing: Theme.unit

                // confirm
                Text {
                    visible: root.phase === "confirm"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Close the day for this store. Every closed shift is consolidated into one journal and the Z report is printed. This runs once per day — a manager may run it early. All shifts must be closed first.")
                    color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                }

                // running
                Text {
                    visible: root.phase === "running"
                    Layout.fillWidth: true
                    text: qsTr("Closing the day…")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
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
                            text: qsTr("Day closed")
                            color: Theme.ok; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge; font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Batch %1").arg(root.batchId)
                            color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                        Text {
                            text: root.provisional ? qsTr("Ledger posting queued — posts on reconnect.")
                                                   : qsTr("Posted to the ledger.")
                            color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                        }
                    }
                }

                // blocked
                ColumnLayout {
                    visible: root.phase === "blocked"
                    Layout.fillWidth: true
                    spacing: Theme.unit
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Cannot close the day — %1 shift(s) still open. Close every shift first, then run End of Day.")
                              .arg(root.openShiftIds.length)
                        color: Theme.warn; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    }
                    Text {
                        visible: root.openShiftIds.length > 0
                        Layout.fillWidth: true
                        text: root.openShiftIds.join("\n")
                        color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // ── Actions ──────────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            // Secondary (Cancel) — only while confirming.
            Button {
                id: cancelBtn
                visible: root.phase === "confirm"
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

            // Primary — label + enabled depend on phase.
            Button {
                id: primaryBtn
                Layout.preferredWidth: 220
                enabled: root.phase !== "running"
                text: root.phase === "confirm" ? qsTr("Run Day Close  (Enter)")
                    : root.phase === "running" ? qsTr("Closing…")
                    : root.phase === "blocked" ? qsTr("Back  (Enter)")
                    : qsTr("Done  (Enter)")
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
                         : root.phase === "blocked" ? Theme.warn
                         : root.phase === "done" ? Theme.ok
                         : Theme.accent
                    border.color: background.color
                }
            }
        }
    }
}
