import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/SettingsPanel.qml — a thin, mostly READ-ONLY settings surface (Tasks launcher). The
// terminal's operating config is resolved server-side and relayed device-domain over the
// Session stream (ConfigService); the terminal does not write company config, so this surface
// reflects the resolved flags rather than editing them. The one genuinely-local, actionable
// setting is the appearance theme (light/dark), which Theme.toggle() switches live. Editing
// company config is a server-admin concern (the server API exists) and is not reachable from
// the terminal contract — surfaced here as a note, not a dead control.
Item {
    id: panel
    signal done()

    function yesNo(b) { return b ? qsTr("Yes") : qsTr("No") }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: qsTr("Settings")
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontLarge
            }
            Button {
                text: qsTr("Close")
                onClicked: panel.done()
            }
        }

        // ── Appearance — the one local, live setting ──────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text {
                Layout.fillWidth: true
                text: qsTr("Appearance")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            Button {
                text: Theme.isDark ? qsTr("Dark — switch to Light") : qsTr("Light — switch to Dark")
                onClicked: Theme.toggle()
            }
        }

        // ── Printer — device-local, like Appearance above ─────────────────────
        // Paper width and auto-print are the DEVICE's own settings, not company
        // config: they live in the engine's local store, apply immediately, and
        // differ legitimately between two tills of the same company. That is the
        // same category as the theme toggle, which is why they are editable here
        // while everything below this divider is not.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text {
                Layout.fillWidth: true
                text: qsTr("Paper width")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            ComboBox {
                id: paperWidth
                model: [58, 80, 104]
                // The engine reports 0 until it has a width; show nothing selected
                // rather than implying a width the printer may not be set to.
                currentIndex: model.indexOf(ConfigService.paperWidthMm)
                onActivated: PosEngineBridge.setDeviceConfig(model[currentIndex], false, false)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text {
                Layout.fillWidth: true
                text: qsTr("Auto-print receipt")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            Switch {
                checked: ConfigService.autoPrintReceipt
                onToggled: PosEngineBridge.setDeviceConfig(0, true, checked)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.unit
            Text {
                Layout.fillWidth: true
                text: qsTr("Test print")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
            Button {
                text: qsTr("Print test page")
                onClicked: PosEngineBridge.printTestPage()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // ── Resolved terminal config (read-only) ──────────────────────────────
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: cfgGrid.implicitHeight
            GridLayout {
                id: cfgGrid
                width: parent.width
                columns: 2
                columnSpacing: Theme.gap
                rowSpacing: Theme.unit

                component Key: Text {
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
                component Val: Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }

                Key { text: qsTr("Config loaded") }
                Val { text: panel.yesNo(ConfigService.loaded) }
                Key { text: qsTr("Currency") }
                Val { text: ConfigService.currencySymbol }
                Key { text: qsTr("Tender complete") }
                Val { text: ConfigService.tenderCompleteMode }
                Key { text: qsTr("Discounts") }
                Val { text: panel.yesNo(ConfigService.allowDiscounts) }
                Key { text: qsTr("Returns") }
                Val { text: panel.yesNo(ConfigService.allowReturns) }
                Key { text: qsTr("Blind return") }
                Val { text: panel.yesNo(ConfigService.allowBlindReturn) }
                Key { text: qsTr("Refund tender") }
                Val { text: ConfigService.refundTenderMode }
                Key { text: qsTr("Partial return") }
                Val { text: panel.yesNo(ConfigService.allowPartialReturn) }
                Key { text: qsTr("Payouts") }
                Val { text: panel.yesNo(ConfigService.payoutEnabled) }
                Key { text: qsTr("Held-cart expiry") }
                Val { text: ConfigService.heldCartExpiry !== "" ? ConfigService.heldCartExpiry : qsTr("end of day") }
                Key { text: qsTr("Payment methods") }
                Val { text: ConfigService.enabledPaymentMethods.length.toString() }
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Company configuration is managed server-side; this terminal shows the resolved settings.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
