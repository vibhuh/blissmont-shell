import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/HistoryPanel.qml — the history context (UX §10), swapped into ItemPanel on F11.
// Recent-list-first with search: opening recalls the most recent finalized bills; arrow + Enter
// on a row reprints it instantly (DUPLICATE), a click opens its view-only detail. Search by
// receipt number opens a bill directly; search by customer refills the list. Detail offers
// reprint, start-return (hands the receipt to the existing returns flow), and back. The engine
// owns every bill (local-first) and the DUPLICATE marking; this panel dispatches reads and
// binds the recent rows / detail to the bridge.
Item {
    id: root

    // Raised when the cashier starts a return from a recalled bill; ItemPanel forwards it to
    // BillingScreen to flip the context to the (already-built) return panel.
    signal navigateReturn()

    HistoryViewModel {
        id: hvm
        bridge: PosEngineBridge
        connection: ConnectionService
    }

    // Re-open recalls the recent list each time the panel is shown (the Loader recreates it).
    Component.onCompleted: hvm.open()

    Connections {
        target: hvm
        function onReturnRequested() { root.navigateReturn() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: hvm.detailActive ? qsTr("History · %1").arg(PosEngineBridge.billDetail.receiptNo)
                                   : qsTr("History")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // Local-only hint — the engine is offline, so older / other-terminal bills won't appear.
        Text {
            Layout.fillWidth: true
            visible: hvm.localOnlyHint
            text: qsTr("Showing local bills only — older or other-terminal bills need connectivity.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        // ── Search (hidden in detail) — receipt no opens a bill, customer fills the list ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            visible: !hvm.detailActive

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                TextField {
                    id: receiptField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Receipt no — exact recall")
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fontBody
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    onAccepted: hvm.searchByReceiptNo(receiptField.text)
                }
                TextField {
                    id: customerField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Customer — search")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    onAccepted: hvm.searchByCustomer(customerField.text)
                }
            }
        }

        // ── Recent / search results list ──────────────────────────────────────
        ListView {
            id: recentList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !hvm.detailActive
            focus: !hvm.detailActive
            model: PosEngineBridge.history
            spacing: Theme.unit
            currentIndex: 0
            keyNavigationEnabled: true

            // The fast path: arrow to a bill, Enter reprints it instantly as a DUPLICATE.
            Keys.onReturnPressed: if (currentItem) hvm.reprint(currentItem.receiptNo)
            Keys.onEnterPressed: if (currentItem) hvm.reprint(currentItem.receiptNo)

            delegate: Rectangle {
                id: billRow
                required property int index
                required property string receiptNo
                required property string total
                required property string settledAt
                required property string customerLabel
                required property bool provisional
                width: ListView.view ? ListView.view.width : 0
                height: rowCol.implicitHeight + Theme.unit * 2
                radius: Theme.radius
                color: ListView.isCurrentItem ? Theme.surfaceAlt : Theme.surface
                border.color: ListView.isCurrentItem ? Theme.accent : Theme.border

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        recentList.currentIndex = billRow.index
                        hvm.openDetail(billRow.receiptNo)   // click → view-only detail
                    }
                }

                ColumnLayout {
                    id: rowCol
                    anchors.fill: parent
                    anchors.margins: Theme.unit
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.unit
                        Text {
                            Layout.fillWidth: true
                            text: billRow.receiptNo + (billRow.provisional ? "  ·  " + qsTr("provisional") : "")
                            color: Theme.text
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                        Text {
                            text: billRow.total
                            color: Theme.text
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontBody
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: billRow.customerLabel + (billRow.settledAt ? "  ·  " + billRow.settledAt : "")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: recentList.count === 0
                text: qsTr("No bills found.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }

        // ── View-only detail of a recalled bill ───────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: hvm.detailActive
            spacing: Theme.unit

            Text {
                Layout.fillWidth: true
                text: (PosEngineBridge.billDetail.customerLabel || qsTr("Walk-in"))
                      + (PosEngineBridge.billDetail.settledAt ? "  ·  " + PosEngineBridge.billDetail.settledAt : "")
                      + (PosEngineBridge.billDetail.provisional ? "  ·  " + qsTr("provisional") : "")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
            }

            // Lines (reuses CartLine roles via the detail's nested CartLineModel).
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: PosEngineBridge.billDetail.lines
                spacing: 2
                delegate: RowLayout {
                    id: lineRow
                    required property string description
                    required property string qty
                    required property string lineTotal
                    width: ListView.view ? ListView.view.width : 0
                    spacing: Theme.unit
                    Text {
                        Layout.fillWidth: true
                        text: lineRow.qty + "  ×  " + lineRow.description
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
            }

            // Payments (reuses Tender roles via the detail's nested TenderListModel).
            Repeater {
                model: PosEngineBridge.billDetail.payments
                delegate: RowLayout {
                    id: payRow
                    required property string method
                    required property string amount
                    required property string reference
                    Layout.fillWidth: true
                    spacing: Theme.unit
                    Text {
                        Layout.fillWidth: true
                        text: payRow.method + (payRow.reference ? "  ·  " + payRow.reference : "")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                    Text {
                        text: payRow.amount
                        color: Theme.textMuted
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontBody
                    }
                }
            }

            // Totals (engine's exact strings).
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.gap
                rowSpacing: 2
                Text { text: qsTr("Subtotal"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                Text { text: PosEngineBridge.billDetail.subtotal || "0.00"; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                Text { text: qsTr("Tax"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
                Text { text: PosEngineBridge.billDetail.taxTotal || "0.00"; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                Text { text: qsTr("Total"); color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge }
                Text { text: PosEngineBridge.billDetail.total || "0.00"; color: Theme.text; font.family: Theme.monoFamily; font.pixelSize: Theme.fontLarge; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
            }

            // Actions on the recalled bill: reprint (DUPLICATE), start return, back.
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.unit
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Reprint (DUPLICATE)")
                    onClicked: hvm.reprint(PosEngineBridge.billDetail.receiptNo)
                }
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Start return")
                    onClicked: hvm.startReturn(PosEngineBridge.billDetail.receiptNo)
                }
                Button {
                    text: qsTr("Back")
                    onClicked: hvm.closeDetail()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: hvm.statusMessage
            visible: hvm.statusMessage !== ""
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
