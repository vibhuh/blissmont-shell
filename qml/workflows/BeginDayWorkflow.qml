import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// workflows/BeginDayWorkflow.qml — the Begin-Register (open session) FULL-SCREEN workflow (UX §12,
// register-session slice contracts v1.10.0).
//
// One workflow, three mode-selected faces (ConfigService.shiftManagementMode), all opening the SAME
// register session underneath (PosEngineBridge.openShift → OpenShift) — cash accountability is
// identical across modes, only the framing differs:
//   • single    — "Open Register": opening float only, no "shift" word. (Reached by auto-open when
//                 no session is open; Tasks hides Begin/Close in single.)
//   • multiple  — today's Begin-Day: opening float, unrestricted. shift_master_id stays empty.
//   • scheduled — suggested (time-matched) shift with an override picker; opening a policy-violating
//                 selection is HELD by the engine (AuthRequired(open_session_policy)) and completed
//                 via the shared supervisor dialog, which re-issues the open with SupervisorAuth.
//
// The ENGINE owns the open (OpenShift → ShiftStateChanged(open), CommandRejected on failure, or
// AuthRequired when a scheduled policy is violated). Keyboard-first: Enter opens, Esc cancels.
Item {
    id: root
    signal closed()

    // Device-default cashier until the shell has a login (see PosEngineBridge::openShift).
    readonly property string cashierId: "cashier-1"
    readonly property string mode: ConfigService.shiftManagementMode  // single | multiple | scheduled

    // entry → opening → done | error | auth
    property string phase: "entry"
    property string errorMessage: ""
    property string openedFloat: ""
    property string authReason: ""

    // Scheduled-mode selection. selectedShiftId is passed as OpenShift.shift_master_id (empty in
    // single/multiple). Retained across an auth re-issue so the same shift + float resend.
    property int    selectedIndex: -1
    property string selectedShiftId: ""
    property string selectedShiftName: ""

    // Active shift definitions (scheduled), sorted by sequence — re-derives if config re-pushes.
    property var shifts: root.activeShifts(ConfigService.shiftMasters)
    // Live clock for the scheduled header (updates while the entry form is up).
    property string clock: Qt.formatTime(new Date(), "hh:mm")

    focus: true
    Component.onCompleted: {
        if (root.mode === "scheduled") {
            var idx = root.suggestIndex()
            if (idx >= 0) root.selectShift(idx)
        }
        floatField.forceActiveFocus()
    }
    // Leaving entry disables the float field; pull focus back to the root so Enter/Esc keep driving
    // the primary/cancel — EXCEPT in auth, where the supervisor dialog owns focus.
    onPhaseChanged: if (root.phase !== "entry" && root.phase !== "auth") root.forceActiveFocus()

    Timer {
        interval: 15000; repeat: true
        running: root.mode === "scheduled" && root.phase === "entry"
        onTriggered: root.clock = Qt.formatTime(new Date(), "hh:mm")
    }

    // ── Scheduled helpers ─────────────────────────────────────────────────────────
    function activeShifts(list) {
        var out = []
        for (var i = 0; i < list.length; i++) if (list[i].isActive) out.push(list[i])
        out.sort(function (a, b) { return (a.sequence || 0) - (b.sequence || 0) })
        return out
    }
    function minutesOf(t) {
        if (!t) return -1
        var p = String(t).split(":")
        return parseInt(p[0]) * 60 + parseInt(p[1])
    }
    // Overnight-aware: a shift whose end ≤ start spans midnight (e.g. Night 22:00–06:00).
    function shiftMatches(s, nowMin) {
        var st = root.minutesOf(s.startTime), en = root.minutesOf(s.endTime)
        if (st < 0 || en < 0) return false
        return en <= st ? (nowMin >= st || nowMin < en) : (nowMin >= st && nowMin < en)
    }
    function suggestIndex() {
        var now = new Date(), nm = now.getHours() * 60 + now.getMinutes()
        for (var i = 0; i < root.shifts.length; i++)
            if (root.shiftMatches(root.shifts[i], nm)) return i
        return root.shifts.length > 0 ? 0 : -1  // fall back to the first shift when none matches now
    }
    function selectShift(i) {
        if (i < 0 || i >= root.shifts.length) return
        root.selectedIndex = i
        root.selectedShiftId = root.shifts[i].id
        root.selectedShiftName = root.shifts[i].name
    }

    // ── Dispatch ──────────────────────────────────────────────────────────────────
    function submit() {
        var v = floatField.text.trim()
        if (v === "") v = "0"
        root.openedFloat = v
        root.phase = "opening"
        // shiftMasterId is empty in single/multiple; the engine only classifies it in scheduled.
        PosEngineBridge.openShift(root.cashierId, v, root.selectedShiftId)
    }
    // Re-issue after AuthRequired(open_session_policy): same shift + float, plus the attestation.
    function reissueWithAuth(reason, authorizedBy) {
        root.phase = "opening"
        PosEngineBridge.openShift(root.cashierId, root.openedFloat, root.selectedShiftId,
                                  reason, authorizedBy)
    }
    function primary() {
        switch (root.phase) {
        case "entry":   root.submit(); break
        case "opening": break                 // waiting on the engine
        case "auth":    break                 // the supervisor dialog owns its actions
        default:        root.closed()         // done / error → leave
        }
    }

    // Single mode has no cancel — the register must be open to sell (it's auto-shown when closed).
    readonly property bool cancellable: root.mode !== "single"

    Keys.onEscapePressed: (e) => { if (root.cancellable) root.closed(); e.accepted = true }
    Keys.onReturnPressed: (e) => { if (root.phase !== "entry" && root.phase !== "auth") { root.primary(); e.accepted = true } }
    Keys.onEnterPressed:  (e) => { if (root.phase !== "entry" && root.phase !== "auth") { root.primary(); e.accepted = true } }

    Connections {
        target: PosEngineBridge
        function onShiftStateChanged(shiftId, status) {
            if (root.phase === "opening" && status === "open") root.phase = "done"
        }
        function onCommandRejected(code, message) {
            if (root.phase !== "opening") return
            root.errorMessage = message !== "" ? message : qsTr("Could not open the register")
            root.phase = "error"
        }
        function onAuthRequired(action, reason) {
            if (root.phase !== "opening" || action !== "open_session_policy") return
            root.authReason = reason
            root.phase = "auth"
        }
    }

    // Opaque full-screen backdrop.
    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.55, 560)
        spacing: Theme.gap

        // ── Heading (mode-framed) ─────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: root.mode === "single" ? qsTr("Open Register")
                    : root.mode === "scheduled" ? qsTr("Begin Shift")
                    : qsTr("Begin Day")
                color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontTotal; font.bold: true
            }
            Text {
                text: root.mode === "single" ? qsTr("Opening cash float")
                    : root.mode === "scheduled" ? qsTr("Now %1 · select shift · opening cash float").arg(root.clock)
                    : qsTr("Open the shift · opening cash float")
                color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // ── Scheduled: shift picker (suggested pre-selected; cashier may override) ──
        Rectangle {
            visible: root.mode === "scheduled" && (root.phase === "entry" || root.phase === "opening")
            Layout.fillWidth: true
            implicitHeight: pickerCol.implicitHeight + 2 * Theme.pad
            radius: Theme.radius; color: Theme.surface; border.color: Theme.border; border.width: 1
            ColumnLayout {
                id: pickerCol
                anchors.fill: parent; anchors.margins: Theme.pad; spacing: 2
                Text {
                    text: qsTr("Shift")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                }
                Text {
                    visible: root.shifts.length === 0
                    Layout.fillWidth: true
                    text: qsTr("No shifts configured — opening an unclassified session.")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                }
                Repeater {
                    model: root.shifts
                    delegate: ListRow {
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        enabled: root.phase === "entry"
                        title: modelData.name + (index === root.suggestIndex() ? qsTr("   · suggested now") : "")
                        subtitle: modelData.startTime + "–" + modelData.endTime
                        selected: index === root.selectedIndex
                        onClicked: root.selectShift(index)
                    }
                }
            }
        }

        // ── Body card (phase-driven) ──────────────────────────────────────────────
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
                    text: qsTr("Opening…")
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
                            text: root.mode === "single" ? qsTr("Register open")
                                : root.selectedShiftName !== "" ? qsTr("%1 shift open").arg(root.selectedShiftName)
                                : qsTr("Shift open")
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

        // ── Actions ───────────────────────────────────────────────────────────────
        RowLayout {
            visible: root.phase !== "auth"
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                id: cancelBtn
                visible: root.phase === "entry" && root.cancellable
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
                Layout.preferredWidth: 240
                enabled: root.phase !== "opening"
                text: root.phase === "opening" ? qsTr("Opening…")
                    : root.phase === "done" ? qsTr("Start Selling  (Enter)")
                    : root.phase === "error" ? qsTr("OK  (Enter)")
                    : root.mode === "single" ? qsTr("Open Register  (Enter)")
                    : root.mode === "scheduled" ? (root.selectedShiftName !== ""
                        ? qsTr("Open %1 Shift  (Enter)").arg(root.selectedShiftName)
                        : qsTr("Open Shift  (Enter)"))
                    : qsTr("Open Shift  (Enter)")
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

    // ── Supervisor authorization (scheduled open-policy gate) ────────────────────
    // Completion half of AuthRequired(open_session_policy): the engine held the open because the
    // selected shift violates the auth policy (too early/late/different/reopen). The supervisor
    // attests and we re-issue the SAME open with SupervisorAuth. Cancelling returns to entry so the
    // cashier can pick the suggested (compliant) shift instead. Shared with the shift-close variance.
    SupervisorAuthDialog {
        active: root.phase === "auth"
        blockedReason: root.authReason
        heading: qsTr("Supervisor authorization · shift")
        confirmText: qsTr("Authorize & Open")
        onAuthorized: (reason, authorizedBy) => root.reissueWithAuth(reason, authorizedBy)
        onCancelled: root.phase = "entry"
    }
}
