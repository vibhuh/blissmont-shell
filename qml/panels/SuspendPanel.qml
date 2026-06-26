import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/SuspendPanel.qml — the suspend/resume context (UX §10), swapped into ItemPanel on F7.
// Two affordances on one panel: park the current cart (instant, optional label) and resume a
// parked draft. Held carts are editable DRAFTS, terminal-local, never synced — kept strictly
// distinct from History (finalized bills). Opening enters resume context: the holds list takes
// focus, so the scan field's bare-number PLU parsing is suspended for that moment (UX §3
// collision-avoidance, same as line-edit / returns). Arrow + Enter on a hold resumes it; the
// engine restores the cart and re-emits CartUpdated (status="held"). The engine owns every
// draft and its expiry; this panel only dispatches and binds the rows to the bridge.
Item {
    id: root

    SuspendResumeViewModel {
        id: srm
        bridge: PosEngineBridge
    }

    // Entering resume context lists the active holds each time the panel is shown.
    Component.onCompleted: srm.openResume()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Text {
            text: qsTr("Suspend / Resume")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }

        // ── Park the current cart (instant, optional label) ───────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            TextField {
                id: labelField
                Layout.fillWidth: true
                placeholderText: qsTr("Label (optional) — e.g. counter 2")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                color: Theme.text
                placeholderTextColor: Theme.textMuted
                onAccepted: { srm.hold(labelField.text); labelField.clear() }
            }
            Button {
                text: qsTr("Suspend sale")
                onClicked: { srm.hold(labelField.text); labelField.clear() }
            }
        }

        Text {
            text: qsTr("Active holds")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }

        // ── Resume list: arrow + Enter resumes the parked draft by id ─────────
        ListView {
            id: holdsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true                 // takes focus → scan-field PLU parsing suspended (UX §3)
            model: PosEngineBridge.heldCarts
            spacing: Theme.unit
            currentIndex: 0
            keyNavigationEnabled: true

            Keys.onReturnPressed: if (currentItem) srm.resume(currentItem.heldCartId)
            Keys.onEnterPressed: if (currentItem) srm.resume(currentItem.heldCartId)

            delegate: Rectangle {
                id: holdRow
                required property int index
                required property string heldCartId
                required property string label
                required property string heldAt
                required property int lineCount
                required property string total
                width: ListView.view ? ListView.view.width : 0
                height: holdCol.implicitHeight + Theme.unit * 2
                radius: Theme.radius
                color: ListView.isCurrentItem ? Theme.surfaceAlt : Theme.surface
                border.color: ListView.isCurrentItem ? Theme.accent : Theme.border

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        holdsList.currentIndex = holdRow.index
                        srm.resume(holdRow.heldCartId)
                    }
                }

                ColumnLayout {
                    id: holdCol
                    anchors.fill: parent
                    anchors.margins: Theme.unit
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.unit
                        Text {
                            Layout.fillWidth: true
                            text: (holdRow.label !== "" ? holdRow.label : qsTr("#%1").arg(holdRow.heldCartId.substring(0, 8)))
                            color: Theme.text
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                        Text {
                            text: holdRow.total
                            color: Theme.text
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontBody
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%1 item(s)").arg(holdRow.lineCount)
                              + (holdRow.heldAt ? "  ·  " + holdRow.heldAt : "")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: holdsList.count === 0
                text: qsTr("No held carts.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }

        Text {
            Layout.fillWidth: true
            text: srm.statusMessage
            visible: srm.statusMessage !== ""
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
