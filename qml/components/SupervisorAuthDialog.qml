import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// components/SupervisorAuthDialog.qml — the ONE supervisor-authorization dialog (register-session
// slice, contracts v1.10.0). Shown inline over a full-screen workflow when the engine holds a gated
// action and emits AuthRequired{action, reason}: an over-tolerance shift-close variance
// (close_shift_variance) OR a SCHEDULED-mode policy-violating open (open_session_policy). It is the
// completion half of that signal — it collects a supervisor attestation and the owning workflow
// RE-ISSUES the original command (CloseShift / OpenShift) carrying SupervisorAuth{reason,
// authorized_by}. The engine trusts the same-device attestation and clears the gate.
//
// Attestation-only (deliberate, 2026-07-04): the shell has no device-side credential store yet — no
// login, no supervisor PIN table — and the engine never verifies a PIN on the wire (SupervisorAuth
// carries only reason + authorized_by). So there is no PIN field here: it would have nothing to
// verify against and nowhere to travel. When a device login/user store lands, add real PIN
// verification here without touching the workflows or the contract.
Item {
    id: dialog
    anchors.fill: parent
    visible: active
    z: 100

    // The owning workflow flips this true when AuthRequired fires and false on authorized()/cancelled().
    property bool active: false
    // The engine's AuthRequired.reason — WHY the action is held (e.g. "Variance ₹120.00 over
    // tolerance ₹50.00" or "Session opens 47m before Morning shift starts"). Shown read-only.
    property string blockedReason: ""
    property string heading: qsTr("Supervisor authorization")
    // Label for the confirm button ("Authorize & Close", "Authorize & Open").
    property string confirmText: qsTr("Authorize")

    signal authorized(string reason, string authorizedBy)
    signal cancelled()

    readonly property bool canSubmit: byField.text.trim() !== "" && reasonField.text.trim() !== ""

    // Reset + focus each time it opens; clear when it closes so a re-open never shows stale entry.
    onActiveChanged: {
        if (active) {
            byField.text = ""
            reasonField.text = ""
            byField.forceActiveFocus()
        }
    }

    function submit() {
        if (!canSubmit) return
        dialog.authorized(reasonField.text.trim(), byField.text.trim())
    }
    function cancel() { dialog.cancelled() }

    // Dim backdrop. Swallows clicks (modal) and a tap on the scrim cancels.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.5
        TapHandler { onTapped: dialog.cancel() }
    }

    // Centered card. focus while visible so Enter/Esc work even before a field is clicked.
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.5, 480)
        implicitHeight: cardCol.implicitHeight + 2 * Theme.pad
        radius: Theme.radius
        color: Theme.surface
        border.width: 1
        border.color: Theme.warn

        // Block backdrop taps from passing through the card.
        TapHandler {}

        ColumnLayout {
            id: cardCol
            anchors.fill: parent
            anchors.margins: Theme.pad
            spacing: Theme.gap

            // ── Heading ──
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                Icon { name: "customer"; color: Theme.warn; size: Theme.fontLarge }
                Text {
                    Layout.fillWidth: true
                    text: dialog.heading
                    color: Theme.text; font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontLarge; font.bold: true
                }
            }

            // ── Why it's held (the engine's reason) ──
            Text {
                visible: dialog.blockedReason !== ""
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: dialog.blockedReason
                color: Theme.warn; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            // ── Authorized by (supervisor id) ──
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: qsTr("Authorized by (supervisor)")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                }
                TextField {
                    id: byField
                    Layout.fillWidth: true
                    placeholderText: qsTr("supervisor id or name")
                    color: Theme.text; placeholderTextColor: Theme.textMuted
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    // Enter moves to the reason field (or submits if reason already filled).
                    onAccepted: reasonField.text.trim() !== "" ? dialog.submit() : reasonField.forceActiveFocus()
                    background: Rectangle {
                        radius: Theme.radiusSmall; color: Theme.surfaceAlt
                        border.color: byField.activeFocus ? Theme.accent : Theme.border
                    }
                }
            }

            // ── Reason ──
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: qsTr("Reason")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                }
                TextField {
                    id: reasonField
                    Layout.fillWidth: true
                    placeholderText: qsTr("why this is authorized")
                    color: Theme.text; placeholderTextColor: Theme.textMuted
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                    onAccepted: dialog.submit()
                    background: Rectangle {
                        radius: Theme.radiusSmall; color: Theme.surfaceAlt
                        border.color: reasonField.activeFocus ? Theme.accent : Theme.border
                    }
                }
            }

            // ── Actions ──
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Button {
                    id: cancelBtn
                    Layout.preferredWidth: 140
                    text: qsTr("Cancel  (Esc)")
                    onClicked: dialog.cancel()
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
                    id: confirmBtn
                    Layout.preferredWidth: 200
                    enabled: dialog.canSubmit
                    text: dialog.confirmText
                    onClicked: dialog.submit()
                    contentItem: Text {
                        text: confirmBtn.text
                        color: confirmBtn.enabled ? Theme.selectionText : Theme.textMuted
                        font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitHeight: Theme.actionButton
                        radius: Theme.radius
                        color: confirmBtn.enabled ? Theme.warn : Theme.surface
                        border.color: confirmBtn.enabled ? Theme.warn : Theme.border
                    }
                }
            }
        }
    }

    // Keyboard: Esc cancels from anywhere; Enter submits when both fields are filled (the fields'
    // own onAccepted also submit, this covers focus sitting on a button).
    Keys.onEscapePressed: (e) => { dialog.cancel(); e.accepted = true }
    Keys.onReturnPressed: (e) => { if (dialog.canSubmit) { dialog.submit(); e.accepted = true } }
    Keys.onEnterPressed:  (e) => { if (dialog.canSubmit) { dialog.submit(); e.accepted = true } }
}
