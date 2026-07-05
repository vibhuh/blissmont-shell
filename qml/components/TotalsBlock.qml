import QtQuick
import QtQuick.Layouts
import Blissmont.Shell

// components/TotalsBlock.qml — zone 2, bottom-right (spec totals block). Two columns:
//   Subtotal / Discount / Taxable value   │   CGST / SGST / Round off
// then a rule, then "N items · M units" and the Total payable. CGST/SGST show for
// intra-state, IGST for inter-state (driven by place of supply, which the accounting
// layer resolves — summary.taxInterstate). Every amount is the engine's exact decimal
// string, rendered verbatim: NO client-side tax math. The split / taxable value /
// round-off / counts are projected over the contract (CartUpdated v1.5.0); until the
// engine populates them they read as 0.00 here — never invented.
Rectangle {
    id: totals
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border

    readonly property var s: PosEngineBridge.summary

    // All amounts route through the one formatter (Phase 1) — grouped, 2-dp, ₹ symbol.
    // A deduction (discount) shows as a negative; the formatter applies its own sign.
    function amt(v) { return Format.money(v) }
    function neg(v) { return (v && v !== "" && v !== "0.00") ? "−" + Format.money(v) : Format.money(v) }

    component Cell: RowLayout {
        property string label: ""
        property string value: ""
        property color valueColor: Theme.text
        property bool mono: true
        Layout.fillWidth: true
        spacing: Theme.unit
        Text {
            text: label
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        Item { Layout.fillWidth: true }
        Text {
            text: value
            color: valueColor
            font.family: mono ? Theme.monoFamily : Theme.fontFamily
            font.pixelSize: Theme.fontBody
            horizontalAlignment: Text.AlignRight
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.unit

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            // ── Left column ───────────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                spacing: Theme.unit
                Cell { label: qsTr("Subtotal");      value: totals.amt(totals.s.subtotal) }
                Cell { label: qsTr("Discount");      value: totals.neg(totals.s.orderDiscount); valueColor: Theme.warn }
                Cell { label: qsTr("Taxable value"); value: totals.amt(totals.s.taxableValue) }
            }

            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.divider }

            // ── Right column — applicable GST pair + round off ────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                spacing: Theme.unit
                Cell { visible: !totals.s.taxInterstate; label: qsTr("CGST"); value: totals.amt(totals.s.cgst) }
                Cell { visible: !totals.s.taxInterstate; label: qsTr("SGST"); value: totals.amt(totals.s.sgst) }
                Cell { visible: totals.s.taxInterstate;  label: qsTr("IGST"); value: totals.amt(totals.s.igst) }
                // Keep the column height even between intra (2 rows) and inter (1 row).
                Item { visible: totals.s.taxInterstate; Layout.fillWidth: true; Layout.preferredHeight: Theme.fontBody + Theme.unit }
                // Round off is a signed adjustment (+/−); show the engine's actual sign.
                Cell { label: qsTr("Round off"); value: totals.amt(totals.s.roundOff); valueColor: Theme.textMuted }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.divider }

        // ── Footer: a 2×2 grid so labels sit on a clean baseline grid with their amounts.
        //    Row 0 — "Total payable" (left) │ the GRAND TOTAL (right), the screen's most
        //    prominent number (Tier 1.2), enlarged/bold, right-aligned and elide-safe.
        //    Row 1 — the item/unit counts (left) │ "You Save" (right, green), which therefore
        //    SHARES the sub-line beside the counts rather than adding a whole new row — so it
        //    costs ~one glyph of height, never spills the fixed strip, and reads directly under
        //    the total it saves against. "You Save" is zero-suppressed (hidden, no "₹0.00").
        GridLayout {
            Layout.fillWidth: true
            // No top margin — the ColumnLayout's own spacing already separates the footer
            // from the divider; a second margin here made the gap above "Total payable"
            // read as too airy versus the tighter rule above it.
            Layout.bottomMargin: Theme.unit
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: 0

            // Row 0 — label ←→ grand total. Both v-centred within the tall number's row so the
            // label sits on the number's optical centre (R2.1 — no top-heavy collision).
            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                text: qsTr("Total payable")
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: Format.money(totals.s.total)
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontGrand
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideLeft
            }

            // Row 1 — counts ←→ You Save.
            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                text: qsTr("%1 items · %2 units")
                        .arg(totals.s.itemCount)
                        .arg(Format.qty(totals.s.unitCount))
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                visible: parseFloat(totals.s.youSave) > 0
                text: qsTr("You Save %1").arg(Format.money(totals.s.youSave))
                color: Theme.ok
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontMedium
                font.bold: true
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideLeft
            }
        }
    }
}
