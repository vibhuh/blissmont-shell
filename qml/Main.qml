import QtQuick
import QtQuick.Controls.Basic
import Blissmont.Shell

// Main.qml — application window. Wires bridge connectivity to ConnectionService at the
// view boundary, installs the frozen F-key keymap (UX §1), and hosts the BillingScreen.
ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    title: qsTr("BlissMont POS")
    color: Theme.bg

    // ── View-boundary wiring: engine connectivity -> ConnectionService ─────────
    Connections {
        target: PosEngineBridge
        function onConnectionChanged() {
            ConnectionService.setConnected(PosEngineBridge.connected)
        }
        function onSyncStatusChanged(online, pending) {
            ConnectionService.applySyncStatus(online, pending)
        }
        // Engine relays device config over the Session stream (contracts v1.1.0). The
        // engine re-pushes it on every (re)connect, so ConfigService rehydrates here
        // with no re-sync logic — the same pattern as sync status above.
        function onConfigUpdated(allowReturns, payoutEnabled, allowDiscounts, tenderCompleteMode, currencySymbol, paymentMethods,
                                 allowBlindReturn, refundTenderMode, returnRequiresAuth, restockDefault, allowPartialReturn, heldCartExpiry) {
            ConfigService.applyConfig(allowReturns, payoutEnabled, allowDiscounts, tenderCompleteMode, currencySymbol, paymentMethods,
                                      allowBlindReturn, refundTenderMode, returnRequiresAuth, restockDefault, allowPartialReturn, heldCartExpiry)
        }
    }

    // Connect to the locally-running engine on startup; on every (re)connect the engine
    // re-pushes the snapshot, so the UI rehydrates with no re-sync logic here (spec §4).
    Component.onCompleted: PosEngineBridge.connectToEngine()

    // ── Frozen keymap (UX §1). Skeleton: nav keys live; others are placeholders. ─
    Shortcut { sequences: ["F12"];    onActivated: billing.focusScan() }       // scan-is-home
    Shortcut { sequences: ["F11"];    onActivated: billing.navState = "history" }
    Shortcut { sequences: ["F6"];     onActivated: billing.navState = "tender" }
    Shortcut { sequences: ["F7"];     onActivated: billing.navState = "suspend" }  // UX §1: F7 = Suspend
    Shortcut { sequences: ["F9"];     onActivated: billing.navState = "return" }
    Shortcut { sequences: ["Ctrl+O"]; onActivated: billing.navState = "payout" }    // UX §1: Ctrl+O = Payout
    Shortcut { sequences: ["Esc"];    onActivated: billing.navState = "item" }

    BillingScreen {
        id: billing
        anchors.fill: parent
    }
}
