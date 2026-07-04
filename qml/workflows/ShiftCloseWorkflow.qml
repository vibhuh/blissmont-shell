import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// workflows/ShiftCloseWorkflow.qml — the Shift-close FULL-SCREEN workflow (UX §12).
//
// Blind denomination count: the cashier keys note/coin counts WITHOUT seeing the expected cash;
// the engine sums the counted total, reconciles it against EXPECTED cash (opening + cash sales +
// cash-in − cash-out − payouts − refunds — the same figure the Z-report prints), computes the
// variance (counted − expected), and reveals the full reconciliation only AFTER commit
// (ShiftClosed, terminal v1.9.0) — blind-count integrity. An over-tolerance variance may be held
// for supervisor sign-off (AuthRequired). Full-screen, never a modal; the engine owns the close
// and the arithmetic. Keyboard-first (Enter in a count field commits).
Item {
    id: root
    signal closed()

    // count → closing → done | auth | error
    //   auth  = engine held an over-tolerance variance (AuthRequired) → supervisor dialog → re-issue
    //   error = the close was rejected for another reason → read-only notice, no re-issue
    property string phase: "count"
    property real   countedTotal: 0
    // Retained from the count so an auth re-issue resends the SAME drawer count + total with the
    // supervisor attestation attached (the engine re-reconciles identically and clears the hold).
    property var    pendingDenoms: []
    property string pendingClosingCash: ""
    property string errorMessage: ""
    // Revealed only in the done phase (blind-count integrity). expectedCash and its breakdown
    // (v1.9.0) let the reveal show what the drawer SHOULD have held and why.
    property string openingFloat: ""
    property string countedCash: ""
    property string variance: ""
    property string expectedCash: ""
    property string cashSales: ""
    property string cashIn: ""
    property string cashOut: ""
    property string payouts: ""
    property string refunds: ""
    property string authReason: ""

    focus: true
    Component.onCompleted: root.forceActiveFocus()
    // Pull focus back to the root when leaving the count phase so Enter/Esc drive the primary/cancel
    // — EXCEPT in the auth phase, where the supervisor dialog owns focus (it grabs its own field).
    onPhaseChanged: if (root.phase !== "count" && root.phase !== "auth") root.forceActiveFocus()

    // Note & coin denominations the drawer holds (blind — no expected shown).
    ListModel {
        id: denomModel
        ListElement { unit: 500; count: 0 }
        ListElement { unit: 200; count: 0 }
        ListElement { unit: 100; count: 0 }
        ListElement { unit: 50;  count: 0 }
        ListElement { unit: 20;  count: 0 }
        ListElement { unit: 10;  count: 0 }
        ListElement { unit: 5;   count: 0 }
        ListElement { unit: 2;   count: 0 }
        ListElement { unit: 1;   count: 0 }
    }

    function recompute() {
        var t = 0
        for (var i = 0; i < denomModel.count; i++) {
            var r = denomModel.get(i)
            t += r.unit * r.count
        }
        root.countedTotal = t
    }

    function commit() {
        if (root.phase !== "count") return
        var denoms = []
        for (var i = 0; i < denomModel.count; i++) {
            var r = denomModel.get(i)
            if (r.count > 0) denoms.push({ unit: r.unit, count: r.count })
        }
        root.pendingDenoms = denoms
        root.pendingClosingCash = String(root.countedTotal)
        root.phase = "closing"
        PosEngineBridge.closeShift(denoms, root.pendingClosingCash)
    }

    // Re-issue the held close with the supervisor attestation (SupervisorAuth). Same drawer count,
    // same total — only the auth is added, so the engine re-reconciles to the same variance and,
    // seeing the attestation, commits instead of holding.
    function reissueWithAuth(reason, authorizedBy) {
        root.phase = "closing"
        PosEngineBridge.closeShift(root.pendingDenoms, root.pendingClosingCash, reason, authorizedBy)
    }

    function primary() {
        switch (root.phase) {
        case "count":   root.commit(); break
        case "closing": break
        case "auth":    break            // the supervisor dialog owns its own actions
        default:        root.closed()    // done / error → leave
        }
    }

    Keys.onEscapePressed: (e) => { root.closed(); e.accepted = true }
    // Enter in a count field commits (the field's onAccepted); in done/error it drives the primary
    // from the root. In the auth phase the supervisor dialog handles keys (it holds focus), so the
    // root stays out of the way.
    Keys.onReturnPressed: (e) => { if (root.phase !== "count" && root.phase !== "auth") { root.primary(); e.accepted = true } }
    Keys.onEnterPressed:  (e) => { if (root.phase !== "count" && root.phase !== "auth") { root.primary(); e.accepted = true } }

    Connections {
        target: PosEngineBridge
        function onShiftClosed(openingFloat, countedCash, variance,
                               expectedCash, cashSales, cashIn, cashOut, payouts, refunds) {
            if (root.phase !== "closing") return
            root.openingFloat = openingFloat
            root.countedCash = countedCash
            root.variance = variance
            root.expectedCash = expectedCash
            root.cashSales = cashSales
            root.cashIn = cashIn
            root.cashOut = cashOut
            root.payouts = payouts
            root.refunds = refunds
            root.phase = "done"
        }
        function onAuthRequired(action, reason) {
            if (root.phase !== "closing" || action !== "close_shift_variance") return
            root.authReason = reason
            root.phase = "auth"
        }
        function onCommandRejected(code, message) {
            if (root.phase !== "closing") return
            root.errorMessage = message !== "" ? message : qsTr("Could not close the shift")
            root.phase = "error"
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.6, 620)
        spacing: Theme.gap

        // ── Heading ──────────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: qsTr("Close Shift")
                color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontTotal; font.bold: true
            }
            Text {
                text: qsTr("Count the drawer · note & coin denominations")
                color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // ── Count table (blind) ──────────────────────────────────────────────────
        Rectangle {
            visible: root.phase === "count" || root.phase === "closing"
            Layout.fillWidth: true
            implicitHeight: countCol.implicitHeight + 2 * Theme.pad
            radius: Theme.radius; color: Theme.surface; border.color: Theme.border; border.width: 1
            ColumnLayout {
                id: countCol
                anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.unit

                Repeater {
                    model: denomModel
                    delegate: RowLayout {
                        required property int index
                        required property int unit
                        required property int count
                        Layout.fillWidth: true
                        spacing: Theme.unit
                        Text {
                            Layout.preferredWidth: 90
                            text: Format.money(String(unit))
                            color: Theme.textMuted; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                        }
                        Text { text: "×"; color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                        TextField {
                            id: countField
                            Layout.preferredWidth: 100
                            enabled: root.phase === "count"
                            placeholderText: "0"
                            horizontalAlignment: Text.AlignRight
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 0 }
                            color: Theme.text; placeholderTextColor: Theme.textMuted
                            font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                            onTextChanged: { denomModel.setProperty(index, "count", parseInt(text) || 0); root.recompute() }
                            onAccepted: root.commit()
                            background: Rectangle {
                                radius: Theme.radiusSmall; color: Theme.surfaceAlt
                                border.color: countField.activeFocus ? Theme.accent : Theme.border
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            Layout.preferredWidth: 130
                            horizontalAlignment: Text.AlignRight
                            text: Format.money(String(unit * count))
                            color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("Counted total")
                        color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: Format.money(String(root.countedTotal))
                        color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontLarge; font.bold: true
                    }
                }
                Text {
                    visible: root.phase === "closing"
                    text: qsTr("Closing the shift…")
                    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                }
            }
        }

        // ── Done: the blind-count reveal ─────────────────────────────────────────
        Rectangle {
            visible: root.phase === "done"
            Layout.fillWidth: true
            implicitHeight: doneCol.implicitHeight + 2 * Theme.pad
            radius: Theme.radius; color: Theme.surface
            border.width: 1
            border.color: Number(root.variance) === 0 ? Theme.ok : Theme.warn
            ColumnLayout {
                id: doneCol
                anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.unit
                Text {
                    text: qsTr("Shift closed")
                    color: Theme.ok; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge; font.bold: true
                }
                // Expected-cash reconciliation: the breakdown that builds Expected, then the
                // Counted total and the variance (counted − expected). Zero-valued movement rows
                // are hidden to keep the reveal readable.
                GridLayout {
                    Layout.fillWidth: true; columns: 2; columnSpacing: Theme.gap; rowSpacing: Theme.unit

                    // — components of expected —
                    Text { text: qsTr("Opening float"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: Format.money(root.openingFloat); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    Text { text: qsTr("Cash sales"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: "+ " + Format.money(root.cashSales); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    Text { visible: Number(root.cashIn) !== 0; text: qsTr("Cash in"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { visible: Number(root.cashIn) !== 0; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: "+ " + Format.money(root.cashIn); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    Text { visible: Number(root.cashOut) !== 0; text: qsTr("Cash out"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { visible: Number(root.cashOut) !== 0; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: "− " + Format.money(root.cashOut); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    Text { visible: Number(root.payouts) !== 0; text: qsTr("Payouts"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { visible: Number(root.payouts) !== 0; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: "− " + Format.money(root.payouts); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    Text { visible: Number(root.refunds) !== 0; text: qsTr("Refunds"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { visible: Number(root.refunds) !== 0; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: "− " + Format.money(root.refunds); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    // — expected (subtotal) —
                    Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }
                    Text { text: qsTr("Expected cash"); color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; font.bold: true }
                    Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: Format.money(root.expectedCash); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; font.bold: true }

                    Text { text: qsTr("Counted cash"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                    Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; text: Format.money(root.countedCash); color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }

                    // — variance = counted − expected —
                    Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }
                    Text { text: qsTr("Variance"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge }
                    Text {
                        Layout.fillWidth: true; horizontalAlignment: Text.AlignRight
                        // Negative = short, positive = over; label it so the sign is unambiguous.
                        text: Format.money(root.variance)
                              + (Number(root.variance) < 0 ? qsTr("  (short)")
                                 : Number(root.variance) > 0 ? qsTr("  (over)") : "")
                        color: Number(root.variance) === 0 ? Theme.ok : Theme.warn
                        font.family: Theme.monoFamily; font.pixelSize: Theme.fontLarge; font.bold: true
                    }
                }
            }
        }

        // ── Error (non-auth rejection) ───────────────────────────────────────────
        Rectangle {
            visible: root.phase === "error"
            Layout.fillWidth: true
            implicitHeight: errorText.implicitHeight + 2 * Theme.pad
            radius: Theme.radius; color: Theme.surface; border.color: Theme.danger; border.width: 1
            Text {
                id: errorText
                anchors.fill: parent; anchors.margins: Theme.pad
                wrapMode: Text.WordWrap
                text: root.errorMessage
                color: Theme.danger; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
            }
        }

        // ── Actions (the auth phase hides these — the supervisor dialog owns its own) ──
        RowLayout {
            visible: root.phase !== "auth"
            Layout.fillWidth: true
            spacing: Theme.gap
            Button {
                id: cancelBtn
                visible: root.phase === "count"
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
                enabled: root.phase !== "closing"
                text: root.phase === "count" ? qsTr("Close Shift  (Enter)")
                    : root.phase === "closing" ? qsTr("Closing…")
                    : root.phase === "error" ? qsTr("OK  (Enter)")
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
                         : root.phase === "error" ? Theme.danger
                         : root.phase === "done" ? Theme.ok
                         : Theme.accent
                    border.color: background.color
                }
            }
        }
    }

    // ── Supervisor authorization (over-tolerance variance hold) ──────────────────
    // The completion half of AuthRequired(close_shift_variance): the engine held the close because
    // the drawer variance exceeds tolerance; the supervisor attests and we re-issue the SAME count
    // with SupervisorAuth. Cancelling abandons the close (the session stays open — the cashier can
    // recount and retry). This is the ONE dialog, shared with the Begin-Register open-policy gate.
    SupervisorAuthDialog {
        active: root.phase === "auth"
        blockedReason: root.authReason
        heading: qsTr("Supervisor authorization · variance")
        confirmText: qsTr("Authorize & Close")
        onAuthorized: (reason, authorizedBy) => root.reissueWithAuth(reason, authorizedBy)
        onCancelled: root.closed()
    }
}
