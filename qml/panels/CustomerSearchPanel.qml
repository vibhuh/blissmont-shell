import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Blissmont.Shell

// panels/CustomerSearchPanel.qml — the customer-select takeover (spec). Opens in the right
// panel (swap), restores home on done. MOBILE-FIRST search (mobile is the unique key) +
// create-new (name, mobile, address, city, PIN, birthday, anniversary, per the old app's
// Create Walk-in Customer). Selecting a result dispatches selectCustomer(id); Walk-in
// clears it (empty id). The engine owns customer state.
//
// DEFERRED DATA: there is no customer-search or create-customer command on the terminal
// contract yet (only select_customer, by id). So search results are empty here and Create
// is staged but not wired; when the contract lands, bind results to the engine and point
// Create at the new command. The select + walk-in paths are live today.
Item {
    id: panel
    signal done()
    property bool creating: false

    function chooseWalkIn() { PosEngineBridge.selectCustomer(""); panel.done() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Customer")
                color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.fontLarge
                Layout.fillWidth: true
            }
            Button { text: qsTr("Walk-in"); onClicked: panel.chooseWalkIn() }
        }

        // ── Search (mobile-first) ────────────────────────────────────────────────
        TextField {
            id: searchField
            Layout.fillWidth: true
            visible: !panel.creating
            placeholderText: qsTr("Search by mobile…")
            inputMethodHints: Qt.ImhDialableCharactersOnly
            font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
            color: Theme.text; placeholderTextColor: Theme.textMuted
            Component.onCompleted: forceActiveFocus()
        }

        // Results (deferred — empty until a customer-search contract lands).
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !panel.creating
            spacing: Theme.unit
            Text {
                Layout.fillWidth: true
                text: searchField.text.trim() === ""
                      ? qsTr("Type a mobile number to search.")
                      : qsTr("No matches. (Live customer search pending engine contract.)")
                color: Theme.textMuted
                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }
            Item { Layout.fillHeight: true }
            Button {
                Layout.fillWidth: true
                text: qsTr("Create new customer")
                onClicked: panel.creating = true
            }
        }

        // ── Create new ───────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: panel.creating
            spacing: Theme.unit

            component Field: TextField {
                Layout.fillWidth: true
                font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody
                color: Theme.text; placeholderTextColor: Theme.textMuted
            }

            Field { placeholderText: qsTr("Name") }
            Field { placeholderText: qsTr("Mobile (unique)"); inputMethodHints: Qt.ImhDialableCharactersOnly }
            Field { placeholderText: qsTr("Address") }
            RowLayout {
                Layout.fillWidth: true; spacing: Theme.unit
                Field { placeholderText: qsTr("City") }
                Field { placeholderText: qsTr("PIN"); inputMethodHints: Qt.ImhDigitsOnly }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: Theme.unit
                Field { placeholderText: qsTr("Birthday") }
                Field { placeholderText: qsTr("Anniversary") }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Create is staged — pending a create-customer command on the engine contract.")
                color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true; spacing: Theme.unit
                Button { Layout.fillWidth: true; text: qsTr("Back"); onClicked: panel.creating = false }
                Button { Layout.fillWidth: true; text: qsTr("Save"); enabled: false }
            }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Cancel")
            onClicked: panel.done()
        }
    }
}
