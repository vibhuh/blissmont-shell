import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/ReturnPanel.qml — the return context (UX §9), swapped into ItemPanel on F9.
// Three steps: load a finalized bill (receipt no, or blind when allowed) → select lines +
// qty + restock → commit the credit note. The engine owns the context and all fiscal math;
// this panel dispatches start/setLineQty/commit and binds the returnable lines + status to
// the bridge. Under refund_tender_mode="both" the cashier picks the refund tender first.
Item {
    id: root

    // The one view-model for this panel. Gating + the two-event status live here; the
    // returnable lines come from the bridge, the policy axes from ConfigService.
    ReturnViewModel {
        id: rvm
        bridge: PosEngineBridge
        config: ConfigService
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: rvm.active ? qsTr("Return · %1").arg(rvm.originalReceiptNo) : qsTr("Return")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // ── Step 1: recall a bill (hidden once a context is loaded) ───────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            visible: !rvm.active

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                Text {
                    text: qsTr("Receipt")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
                TextField {
                    id: receiptField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Original receipt no")
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontBody
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    onAccepted: rvm.startReturn(receiptField.text, blindCheck.checked)
                }
            }
            CheckBox {
                id: blindCheck
                visible: rvm.allowBlind
                text: qsTr("Blind return (no original lookup)")
                contentItem: Text {
                    text: blindCheck.text
                    leftPadding: blindCheck.indicator.width + Theme.unit
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Start return")
                enabled: receiptField.text.trim() !== "" || blindCheck.checked
                onClicked: rvm.startReturn(receiptField.text, blindCheck.checked)
            }
        }

        // ── Refund tender choice (refund_tender_mode="both") ──────────────────
        // The cashier picks where the whole refund goes; commit stays disabled until then.
        ColumnLayout {
            Layout.fillWidth: true
            visible: rvm.active && rvm.needsRefundChoice
            spacing: Theme.unit
            Text {
                text: qsTr("Refund to")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                Repeater {
                    model: [{ key: "original", label: qsTr("Original tender") },
                            { key: "cash", label: qsTr("Cash") }]
                    delegate: Button {
                        id: choiceBtn
                        required property var modelData
                        Layout.fillWidth: true
                        text: choiceBtn.modelData.label
                        highlighted: rvm.refundChoice === choiceBtn.modelData.key
                        onClicked: rvm.setRefundChoice(choiceBtn.modelData.key)
                        contentItem: Text {
                            text: choiceBtn.text
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: choiceBtn.highlighted ? Theme.surfaceAlt : Theme.surface
                            radius: Theme.radius
                            border.color: choiceBtn.highlighted ? Theme.accent : Theme.border
                        }
                    }
                }
            }
        }

        // ── Step 2: returnable lines (engine snapshot; edits re-emit) ─────────
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: rvm.active
            model: PosEngineBridge.returnLines
            spacing: Theme.unit
            delegate: ColumnLayout {
                id: lineRow
                required property int originalLineNo
                required property string description
                required property string soldQty
                required property string refundableQty
                required property string selectedQty
                required property string lineTotal
                required property bool restock
                width: ListView.view ? ListView.view.width : 0
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.unit
                    Text {
                        Layout.fillWidth: true
                        text: lineRow.description
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                    Text {
                        text: lineRow.lineTotal
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontBody
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.unit
                    Text {
                        text: qsTr("of %1 (refundable %2)").arg(lineRow.soldQty).arg(lineRow.refundableQty)
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                    }
                    Item { Layout.fillWidth: true }
                    TextField {
                        id: qtyField
                        Layout.preferredWidth: 80
                        text: lineRow.selectedQty
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontBody
                        color: Theme.text
                        horizontalAlignment: Text.AlignRight
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onEditingFinished: rvm.setLineQty(lineRow.originalLineNo, qtyField.text, restockSwitch.checked)
                    }
                    CheckBox {
                        id: restockSwitch
                        checked: lineRow.restock
                        text: qsTr("Restock")
                        onToggled: rvm.setLineQty(lineRow.originalLineNo, qtyField.text, restockSwitch.checked)
                        contentItem: Text {
                            text: restockSwitch.text
                            leftPadding: restockSwitch.indicator.width + Theme.unit
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
            }
        }

        // ── Step 3: commit the credit note ────────────────────────────────────
        Button {
            id: commitBtn
            Layout.fillWidth: true
            visible: rvm.active
            text: qsTr("Commit return  (credit note)")
            enabled: rvm.canCommit
            onClicked: rvm.commit()
            contentItem: Text {
                text: commitBtn.text
                color: commitBtn.enabled ? Theme.text : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: commitBtn.enabled ? Theme.ok : Theme.surface
                radius: Theme.radius
                border.color: commitBtn.enabled ? Theme.ok : Theme.border
            }
        }

        Text {
            Layout.fillWidth: true
            text: rvm.statusMessage
            visible: rvm.statusMessage !== ""
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
