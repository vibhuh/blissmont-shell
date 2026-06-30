import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/TenderPanel.qml — the tender context (UX §6), swapped into ItemPanel on F12.
//
// A CALCULATOR, not a form (refinement brief, Phase 2). The common case — cash — is one
// field: open → Received prefilled to the balance and pre-selected → Enter (exact) or type
// the tendered amount → Enter completes the sale. No mouse, no tabbing. Change is read-only,
// computed live from Received − Total (engine owns the authoritative settle). Other methods
// are progressive disclosure: a row of choices; selecting Card/UPI reveals its reference
// field; partial tenders accumulate as a split until the balance clears. The engine owns all
// money — this panel only dispatches tender/complete and binds totals to the bridge.
Item {
    id: root

    TenderViewModel {
        id: tvm
        bridge: PosEngineBridge
        config: ConfigService
    }

    readonly property var summary: PosEngineBridge.summary
    // The method the Received field currently tenders (defaults to the cash path).
    property string activeMethod: ""
    property string activeRefMode: "none"
    readonly property bool activeIsCash: activeMethod !== "" &&
                                         activeMethod.toLowerCase() === "cash"

    // Prefill Received with the remaining balance (== the bill total on a fresh tender),
    // as a plain 2-dp value the engine can parse — never raw precision.
    function prefill() { amountField.text = Format.plain(root.summary.balanceDue) }

    // Switch the active payment method (a choice click or hotkey). Reveals the reference
    // field when the method needs one, and lands the cursor on the field to type next.
    function activate(method, refMode) {
        root.activeMethod = method
        root.activeRefMode = refMode
        referenceField.text = ""
        root.prefill()
        if (refMode !== "none") {
            referenceField.forceActiveFocus()
        } else {
            amountField.forceActiveFocus()
            amountField.selectAll()
        }
    }

    // Enter on Received (or on the reference field): tender the active method and complete
    // when the balance clears. A required-but-blank reference parks the cursor there first.
    function commit() {
        if (root.activeRefMode === "required" && referenceField.text.trim() === "") {
            referenceField.forceActiveFocus()
            return
        }
        tvm.tenderAndComplete(root.activeMethod, amountField.text, referenceField.text)
        referenceField.text = ""
    }

    Component.onCompleted: {
        root.activeMethod = tvm.primaryMethod
        root.activeRefMode = tvm.referenceModeFor(tvm.primaryMethod)
        prefill()
        amountField.forceActiveFocus()
        amountField.selectAll()
    }

    // Per-method hotkeys — panel-scoped, live only while the tender panel is loaded (the
    // global F-keys stay owned by Main.qml).
    Repeater {
        model: ConfigService.enabledPaymentMethods
        delegate: Item {
            id: hotkeyItem
            required property var modelData
            Shortcut {
                sequence: hotkeyItem.modelData.hotkey
                enabled: hotkeyItem.modelData.hotkey !== ""
                onActivated: root.activate(hotkeyItem.modelData.method,
                                           hotkeyItem.modelData.referenceMode)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Tender")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // ── The calculator: Total (read-only) / Received (editable) / Change (read-only) ──
        Rectangle {
            Layout.fillWidth: true
            color: Theme.surfaceAlt
            radius: Theme.radius
            border.color: Theme.border
            implicitHeight: calcCol.implicitHeight + 2 * Theme.pad

            GridLayout {
                id: calcCol
                anchors.fill: parent
                anchors.margins: Theme.pad
                columns: 2
                columnSpacing: Theme.gap
                rowSpacing: Theme.gap

                Text { text: qsTr("Total"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; Layout.alignment: Qt.AlignVCenter }
                Text {
                    text: Format.money(root.summary.total)
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontLarge
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                }

                Text { text: qsTr("Received"); color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; Layout.alignment: Qt.AlignVCenter }
                TextField {
                    id: amountField
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontLarge
                    color: Theme.text
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    selectByMouse: true
                    // Commit-format to a plain 2-dp value (600 → 600.00, 600.567 → 600.57)
                    // when focus leaves; Enter completes before this matters.
                    onEditingFinished: if (text.trim() !== "") text = Format.plain(text)
                    onAccepted: root.commit()
                    background: Rectangle {
                        color: Theme.surface
                        radius: Theme.radius
                        border.color: amountField.activeFocus ? Theme.accent : Theme.border
                    }
                }

                Text { text: qsTr("Change"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; Layout.alignment: Qt.AlignVCenter }
                Text {
                    id: changeText
                    readonly property string change: tvm.changeDuePreview(amountField.text)
                    text: Format.money(change)
                    color: change !== "0.00" ? Theme.ok : Theme.textMuted
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontLarge
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                }

                // Reference (manual UPI etc.) — revealed only when the active method asks.
                Text {
                    visible: root.activeRefMode !== "none"
                    text: qsTr("Reference")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    Layout.alignment: Qt.AlignVCenter
                }
                TextField {
                    id: referenceField
                    visible: root.activeRefMode !== "none"
                    Layout.fillWidth: true
                    placeholderText: root.activeRefMode === "required"
                                     ? qsTr("required") : qsTr("optional")
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontBody
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    onAccepted: root.commit()
                }
            }
        }

        // ── Payment-method choices (cash is the default; pick another to switch) ──────
        Flow {
            Layout.fillWidth: true
            spacing: Theme.unit
            Repeater {
                model: ConfigService.enabledPaymentMethods
                delegate: Button {
                    id: methodBtn
                    required property var modelData
                    readonly property bool active: root.activeMethod === methodBtn.modelData.method
                    text: (methodBtn.modelData.hotkey ? "[" + methodBtn.modelData.hotkey + "]  " : "")
                          + methodBtn.modelData.displayName
                    onClicked: root.activate(methodBtn.modelData.method,
                                             methodBtn.modelData.referenceMode)
                    contentItem: Text {
                        text: methodBtn.text
                        color: methodBtn.active ? Theme.selectionText : Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitHeight: 36
                        color: methodBtn.active ? Theme.selectionBg : Theme.surface
                        radius: Theme.radius
                        border.color: methodBtn.active ? Theme.accent : Theme.border
                    }
                }
            }
            Text {
                visible: ConfigService.enabledPaymentMethods.length === 0
                text: qsTr("No payment methods configured.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }

        // ── Split: tenders taken so far, each removable (only when a split is building) ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: tenderList.count > 0
            spacing: Theme.unit

            Text {
                text: qsTr("Tendered")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
            }
            ListView {
                id: tenderList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: PosEngineBridge.tenders
                spacing: Theme.unit
                delegate: RowLayout {
                    id: tenderRow
                    required property int tenderNo
                    required property string method
                    required property string amount
                    required property string reference
                    width: ListView.view ? ListView.view.width : 0
                    spacing: Theme.unit
                    Text {
                        Layout.fillWidth: true
                        text: tenderRow.method + (tenderRow.reference ? " · " + tenderRow.reference : "")
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                    Text {
                        text: Format.money(tenderRow.amount)
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontBody
                    }
                    Button {
                        text: "✕"
                        onClicked: tvm.removeTender(tenderRow.tenderNo)
                    }
                }
            }
            // Balance still due on a building split.
            RowLayout {
                Layout.fillWidth: true
                visible: Format.money(root.summary.balanceDue) !== Format.money("0")
                Text { text: qsTr("Balance"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                Item { Layout.fillWidth: true }
                Text { text: Format.money(root.summary.balanceDue); color: Theme.warn; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody }
            }
        }

        // Keep the calculator anchored to the top when no split list is showing.
        Item { Layout.fillHeight: tenderList.count === 0 }

        // ── Complete (Enter on the amount usually settles before this is needed) ──────
        Button {
            id: completeBtn
            Layout.fillWidth: true
            text: qsTr("Complete  (Enter)")
            enabled: tvm.canComplete
            onClicked: tvm.complete()
            contentItem: Text {
                text: completeBtn.text
                color: completeBtn.enabled ? Theme.selectionText : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitHeight: Theme.actionButton
                color: completeBtn.enabled ? Theme.accent : Theme.surface
                radius: Theme.radius
                border.color: completeBtn.enabled ? Theme.accent : Theme.border
            }
        }

        Text {
            Layout.fillWidth: true
            text: tvm.statusMessage
            visible: tvm.statusMessage !== ""
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
