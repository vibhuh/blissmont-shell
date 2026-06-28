import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// panels/ItemPanel.qml — the right-panel host (spec). HOME state is product search; tender,
// customer select, bill discount, qty/price override, return, history, suspend and payout
// are TEMPORARY TAKEOVERS that restore the search home-state on done/cancel. The host
// (BillingScreen) drives `context`; takeover panels ask to restore home via navigate("item").
Rectangle {
    id: panel
    property string context: "item"
    property var vm: null   // BillingViewModel, threaded to the product-search home state
    // A loaded panel asks the host to switch context (e.g. history → return seam, or a
    // takeover restoring the "item" home-state).
    signal navigate(string context)

    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    // HOME — product search (the panel returns here after every takeover).
    Loader {
        anchors.fill: parent
        active: panel.context === "item"
        visible: active
        sourceComponent: ProductSearchPanel { vm: panel.vm }
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "customer"
        visible: active
        sourceComponent: CustomerSearchPanel {
            onDone: panel.navigate("item")
        }
    }

    Loader {
        anchors.fill: parent
        active: panel.context === "billdiscount"
        visible: active
        sourceComponent: BillDiscountPanel {
            onDone: panel.navigate("item")
        }
    }

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

    // Fallback placeholder for contexts that swap in here as later slices land (e.g. misc).
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap
        visible: panel.context !== "item" && panel.context !== "customer"
                 && panel.context !== "billdiscount" && panel.context !== "tender"
                 && panel.context !== "return" && panel.context !== "history"
                 && panel.context !== "suspend" && panel.context !== "payout"

        Text {
            text: qsTr("Panel: %1").arg(panel.context)
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLarge
        }
        Text {
            text: qsTr("Separate build — swaps in here as the slice lands.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true }
    }
}
