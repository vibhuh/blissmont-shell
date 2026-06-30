pragma Singleton
import QtQuick
import Blissmont.Shell

// theme/Theme.qml — design tokens (spec §2): 8pt spacing, IBM Plex typography, colors.
// Single source of truth for visual constants; every component reads from here.
//
// LIGHT / DARK (SHELL_KEYBOARD_LOOKUP brief, Part 2): the shell was dark-only; retail counters
// are often bright-lit. Colors now resolve through `mode` (light | dark) over two CONSIDERED
// palettes (not a naive invert) — each with its own text/border/accent and, critically, its own
// selected-row highlight so the keyboard lookup's highlight (Part 1) stays clearly visible in
// both. `mode` DEFAULTS from Configuration (ConfigService.themeMode) and is live-switchable
// without restart: Theme.toggle() (or any assignment to `mode`) breaks the config binding and
// flips every bound color instantly. User intent wins — a later config rehydrate won't stomp a
// manual toggle.
QtObject {
    id: theme

    // Active palette. Initial value binds to the configured default; assigning it (toggle())
    // switches live and detaches from config (so the user's choice sticks for the session).
    property string mode: ConfigService.themeMode      // "light" | "dark"
    function toggle() { mode = (mode === "light" ? "dark" : "light") }
    readonly property bool isDark: mode === "dark"

    // Spacing — 8pt scale. A single, consistent scale (Phase 3): use these everywhere so
    // density reads evenly. xs=4 · sm=8 · md=12 · lg=16 · xl=24 · xxl=32.
    readonly property int spaceXs: 4
    readonly property int unit: 8      // sm
    readonly property int spaceMd: 12
    readonly property int gap: 16      // lg
    readonly property int pad: 16
    readonly property int spaceXl: 24
    readonly property int spaceXxl: 32
    readonly property int radius: 6
    readonly property int radiusSmall: 4

    // Interaction sizing.
    readonly property int touchMin: 64   // grid quick-key minimum touch target (spec §quick-keys)
    readonly property int actionButton: 56 // bottom action-bar uniform button edge
    readonly property int iconButton: 32  // inline icon control (scope toggle, list/grid toggle)
    readonly property int chipHeight: 30  // category chip
    readonly property int rowHeight: 40   // bill-grid row (Phase 3: even table density)

    // Icon sizing (one icon family — components/Icon.qml). Stroke-based, themed.
    readonly property int iconSm: 16
    readonly property int iconMd: 20
    readonly property int iconLg: 24

    // ── Palettes ─────────────────────────────────────────────────────────────
    // Dark — the original values, kept as the dark palette.
    readonly property QtObject dark: QtObject {
        readonly property color bg: "#0f1115"
        readonly property color surface: "#1a1d23"
        readonly property color surfaceAlt: "#23272f"
        readonly property color border: "#2a2f37"
        readonly property color divider: "#21262e"   // subtle in-panel separator (lighter than border)
        readonly property color text: "#e8eaed"
        readonly property color textMuted: "#9aa0a6"
        readonly property color accent: "#3b82f6"
        readonly property color ok: "#22c55e"
        readonly property color warn: "#f59e0b"
        readonly property color danger: "#ef4444"
        readonly property color selectionBg: "#27406b"   // keyboard-highlight band (clear on dark)
        readonly property color selectionText: "#ffffff"
    }
    // Light — its own considered values (contrast-checked), not an inversion.
    readonly property QtObject light: QtObject {
        readonly property color bg: "#f5f6f8"
        readonly property color surface: "#ffffff"
        readonly property color surfaceAlt: "#e7eaef"
        readonly property color border: "#d2d6dc"
        readonly property color divider: "#eaedf1"   // subtle in-panel separator (lighter than border)
        readonly property color text: "#15181d"
        readonly property color textMuted: "#5b6168"
        readonly property color accent: "#2563eb"
        readonly property color ok: "#16a34a"
        readonly property color warn: "#b45309"
        readonly property color danger: "#dc2626"
        readonly property color selectionBg: "#cfe0fb"   // keyboard-highlight band (clear on light)
        readonly property color selectionText: "#0b2e6b"
    }
    readonly property QtObject palette: isDark ? dark : light

    // ── Color tokens (resolve through the active palette) ─────────────────────
    readonly property color bg: palette.bg
    readonly property color surface: palette.surface
    readonly property color surfaceAlt: palette.surfaceAlt
    readonly property color border: palette.border
    readonly property color divider: palette.divider
    readonly property color text: palette.text
    readonly property color textMuted: palette.textMuted
    readonly property color accent: palette.accent
    readonly property color ok: palette.ok
    readonly property color warn: palette.warn
    readonly property color danger: palette.danger
    // Selected-row highlight for the keyboard lookup (Part 1) — visible in BOTH modes.
    readonly property color selectionBg: palette.selectionBg
    readonly property color selectionText: palette.selectionText

    // Typography — IBM Plex (spec §2); system fallback if unavailable.
    readonly property string fontFamily: "IBM Plex Sans"
    readonly property string monoFamily: "IBM Plex Mono"
    readonly property int fontSmall: 12   // sublines (HSN·GST%), chips, secondary labels
    readonly property int fontBody: 14
    readonly property int fontMedium: 16  // emphasised body (item name, customer name)
    readonly property int fontLarge: 20
    readonly property int fontTotal: 28
    readonly property int fontGrand: 42   // the Grand Total — the most prominent number on screen (Tier 1.2: ~24% larger)
}
