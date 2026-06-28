pragma Singleton
import QtQuick

// theme/Theme.qml — design tokens (spec §2): 8pt spacing, IBM Plex typography, colors.
// Single source of truth for visual constants; every component reads from here.
QtObject {
    // Spacing — 8pt scale.
    readonly property int unit: 8
    readonly property int gap: 16
    readonly property int pad: 16
    readonly property int radius: 6
    readonly property int radiusSmall: 4

    // Interaction sizing.
    readonly property int touchMin: 64   // grid quick-key minimum touch target (spec §quick-keys)
    readonly property int actionButton: 56 // bottom action-bar uniform button edge
    readonly property int iconButton: 32  // inline icon control (scope toggle, list/grid toggle)
    readonly property int chipHeight: 30  // category chip

    // Color tokens.
    readonly property color bg: "#0f1115"
    readonly property color surface: "#1a1d23"
    readonly property color surfaceAlt: "#23272f"
    readonly property color border: "#2a2f37"
    readonly property color text: "#e8eaed"
    readonly property color textMuted: "#9aa0a6"
    readonly property color accent: "#3b82f6"
    readonly property color ok: "#22c55e"
    readonly property color warn: "#f59e0b"
    readonly property color danger: "#ef4444"

    // Typography — IBM Plex (spec §2); system fallback if unavailable.
    readonly property string fontFamily: "IBM Plex Sans"
    readonly property string monoFamily: "IBM Plex Mono"
    readonly property int fontSmall: 12   // sublines (HSN·GST%), chips, secondary labels
    readonly property int fontBody: 14
    readonly property int fontLarge: 20
    readonly property int fontTotal: 28
}
