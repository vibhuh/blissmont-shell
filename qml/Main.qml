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
                                 allowBlindReturn, refundTenderMode, returnRequiresAuth, restockDefault, allowPartialReturn, heldCartExpiry,
                                 payoutCategories, storeName, registerName) {
            ConfigService.applyConfig(allowReturns, payoutEnabled, allowDiscounts, tenderCompleteMode, currencySymbol, paymentMethods,
                                      allowBlindReturn, refundTenderMode, returnRequiresAuth, restockDefault, allowPartialReturn, heldCartExpiry,
                                      payoutCategories, storeName, registerName)
        }
    }

    // Connect to the locally-running engine on startup; on every (re)connect the engine
    // re-pushes the snapshot, so the UI rehydrates with no re-sync logic here (spec §4).
    Component.onCompleted: PosEngineBridge.connectToEngine()

    // The one formatting standard (refinement brief, Phase 1): keep the Format singleton's
    // currency symbol in lockstep with the configured one, so every formatted amount across
    // the UI tracks a config push with no other wiring.
    Binding {
        target: Format
        property: "currencySymbol"
        value: ConfigService.currencySymbol
    }

    // ── Frozen keymap (spec action bar). Each maps to BillingScreen.doAction, the same
    //    handler the on-screen icon buttons call, so button and key stay in lockstep. ──
    Shortcut { sequences: ["F3"];     onActivated: billing.doAction("clear") }    // Void
    Shortcut { sequences: ["F6"];     onActivated: billing.doAction("sundry") }   // Sundry
    Shortcut { sequences: ["F7"];     onActivated: billing.doAction("hold") }     // Hold
    Shortcut { sequences: ["F9"];     onActivated: billing.doAction("return") }   // Return (via History seam too)
    Shortcut { sequences: ["F10"];    onActivated: billing.openTasks() }          // Tasks launcher
    Shortcut { sequences: ["F11"];    onActivated: billing.doAction("history") }  // History
    Shortcut { sequences: ["F12"];    onActivated: billing.doAction("charge") }   // Charge
    // Bill-level discount opens the right-panel takeover (on the bar as Discount).
    Shortcut { sequences: ["Ctrl+D"]; onActivated: billing.doAction("discount") }
    // Payout — gated on payout_enabled (engine relays it via ConfigService); inert if off.
    Shortcut { sequences: ["Ctrl+O"]; onActivated: billing.doAction("payout") }
    // Esc returns the right panel to its product-search home-state (re-focuses search).
    Shortcut { sequences: ["Esc"];    onActivated: billing.focusSearch() }

    BillingScreen {
        id: billing
        anchors.fill: parent
    }
}
