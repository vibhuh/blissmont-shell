import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/TenderPanel.qml — the tender context (UX §6), swapped into ItemPanel on F6.
// Keyboard-first: each enabled method is activatable by its explicit hotkey (from the
// contract — survives localization and Cash/Card collisions). The engine owns all money;
// this panel only dispatches add/remove/complete and binds totals/tenders to the bridge.
Item {
    id: root

    // The one view-model for this panel. Reference gating + balance-zero completion live
    // here; methods/mode come from ConfigService, tenders/totals from the bridge.
    TenderViewModel {
        id: tvm
        bridge: PosEngineBridge
        config: ConfigService
    }

    property string selectedMethod: ""
    property string selectedRefMode: "none"

    function selectMethod(row) {
        root.selectedMethod = row.method
        root.selectedRefMode = row.referenceMode
        amountField.text = PosEngineBridge.summary.balanceDue   // default: tender the remaining
        referenceField.text = ""
        if (root.selectedRefMode !== "none")
            referenceField.forceActiveFocus()
        else
            amountField.forceActiveFocus()
    }

    // Per-method hotkeys — panel-scoped, so they are live only while the tender panel is
    // loaded (the global F-keys stay owned by Main.qml).
    Repeater {
        model: ConfigService.enabledPaymentMethods
        delegate: Item {
            id: hotkeyItem
            required property var modelData
            Shortcut {
                sequence: hotkeyItem.modelData.hotkey
                enabled: hotkeyItem.modelData.hotkey !== ""
                onActivated: root.selectMethod(hotkeyItem.modelData)
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

        // ── Enabled methods (sorted; hotkey + label) ──────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Repeater {
                model: ConfigService.enabledPaymentMethods
                delegate: Button {
                    id: methodBtn
                    required property var modelData
                    Layout.fillWidth: true
                    text: (methodBtn.modelData.hotkey ? "[" + methodBtn.modelData.hotkey + "]  " : "") + methodBtn.modelData.displayName
                    highlighted: root.selectedMethod === methodBtn.modelData.method
                    onClicked: root.selectMethod(methodBtn.modelData)
                    contentItem: Text {
                        text: methodBtn.text
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: methodBtn.highlighted ? Theme.surfaceAlt : Theme.surface
                        radius: Theme.radius
                        border.color: methodBtn.highlighted ? Theme.accent : Theme.border
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

        // ── Amount + reference (reference shown per the method's reference_mode) ─
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text {
                text: qsTr("Amount")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            TextField {
                id: amountField
                Layout.fillWidth: true
                enabled: root.selectedMethod !== ""
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontBody
                color: Theme.text
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }
        }

        TextField {
            id: referenceField
            Layout.fillWidth: true
            // Manual reference capture (manual-UPI etc.): shown when the method asks for it.
            visible: root.selectedRefMode !== "none"
            placeholderText: root.selectedRefMode === "required"
                             ? qsTr("Reference (required)")
                             : qsTr("Reference (optional)")
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontBody
            color: Theme.text
            placeholderTextColor: Theme.textMuted
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Add tender")
            enabled: root.selectedMethod !== "" &&
                     !(root.selectedRefMode === "required" && referenceField.text.trim() === "")
            onClicked: {
                tvm.addTender(root.selectedMethod, amountField.text, referenceField.text)
                referenceField.text = ""
            }
        }

        // ── Running tenders, each removable ───────────────────────────────────
        ListView {
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
                    text: tenderRow.amount
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

        // ── Totals (engine's exact strings) ───────────────────────────────────
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.unit
            Text { text: qsTr("Total");     color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
            Text { text: PosEngineBridge.summary.total || "0.00"; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.alignment: Qt.AlignRight; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
            Text { text: qsTr("Tendered");  color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
            Text { text: PosEngineBridge.summary.amountTendered || "0.00"; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
            Text { text: qsTr("Balance");   color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
            Text { text: PosEngineBridge.summary.balanceDue || "0.00"; color: Theme.warn; font.family: Theme.monoFamily; font.pixelSize: Theme.fontLarge; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
            Text { text: qsTr("Change");    color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody; visible: changeText.text !== "0.00" && changeText.text !== "" }
            Text { id: changeText; text: PosEngineBridge.summary.changeDue || "0.00"; color: Theme.ok; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; visible: text !== "0.00" && text !== ""; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
        }

        Button {
            id: completeBtn
            Layout.fillWidth: true
            text: qsTr("Complete  (balance zero)")
            enabled: tvm.canComplete
            onClicked: tvm.complete()
            contentItem: Text {
                text: completeBtn.text
                color: completeBtn.enabled ? Theme.text : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: completeBtn.enabled ? Theme.ok : Theme.surface
                radius: Theme.radius
                border.color: completeBtn.enabled ? Theme.ok : Theme.border
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

    // Completion policy: in "auto" mode the engine settles as soon as the balance hits
    // zero (with something actually tendered); "confirm" requires the Complete action.
    Connections {
        target: tvm
        function onStateChanged() {
            if (tvm.autoComplete && tvm.canComplete &&
                PosEngineBridge.summary.amountTendered !== "" &&
                PosEngineBridge.summary.amountTendered !== "0.00") {
                tvm.complete()
            }
        }
    }
}
