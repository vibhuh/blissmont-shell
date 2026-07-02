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
// contract yet (only select_customer, by id). So search results are empty here, and the
// CREATE-NEW affordance is intentionally absent (R1.4) — a create form would be a dead end
// with no engine command to save it. When a create-customer command lands on the contract,
// re-introduce the create path here and point Save at it. The select + walk-in paths are
// live today.
Item {
    id: panel
    signal done()

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
            placeholderText: qsTr("Search by mobile…")
            inputMethodHints: Qt.ImhDialableCharactersOnly
            font.family: Theme.monoFamily; font.pixelSize: Theme.fontBody
            color: Theme.text; placeholderTextColor: Theme.textMuted
            Component.onCompleted: forceActiveFocus()
        }

        // Results (deferred — empty until a customer-search contract lands). Create-new is
        // intentionally omitted (R1.4): no create-customer command exists on the engine
        // contract yet, so there is no dead-end create form to fall into.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Cancel")
            onClicked: panel.done()
        }
    }
}
