import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// panels/ItemPanel.qml — the persistent right-panel placeholder (spec §6).
// In the skeleton it shows the active context label; post-skeleton each context
// (item / tender / payout / history / return) becomes its own panel swapped in here.
Rectangle {
    id: panel
    property string context: "item"
    // A loaded panel asks the host (BillingScreen) to switch context — e.g. history hands a
    // recalled bill to the return flow. BillingScreen sets navState from this.
    signal navigate(string context)

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    // The tender (F6), return (F9), and history (F11) contexts are real panels; the rest are
    // still placeholders that swap in here as each context lands.
    Loader {
        anchors.fill: parent
        active: panel.context === "tender"
        visible: active
        sourceComponent: TenderPanel {}
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "return"
        visible: active
        sourceComponent: ReturnPanel {}
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "history"
        visible: active
        sourceComponent: HistoryPanel {
            onNavigateReturn: panel.navigate("return")
        }
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "suspend"
        visible: active
        sourceComponent: SuspendPanel {}
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "payout"
        visible: active
        sourceComponent: PayoutPanel {}
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap
        visible: panel.context !== "tender" && panel.context !== "return"
                 && panel.context !== "history" && panel.context !== "suspend"
                 && panel.context !== "payout"

        Text {
            text: qsTr("Panel: %1").arg(panel.context)
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }
        Text {
            text: qsTr("Placeholder — context panels swap in here.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true }
    }
}
