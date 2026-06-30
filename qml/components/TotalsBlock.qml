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

        // ── Footer: the GRAND TOTAL — the screen's most prominent number (Tier 1.2). It is
        //    enlarged and bold, gets the row's full width on the right so it never crowds the
        //    label or clips, and sits in its own padded band so the big number has comfortable
        //    baseline room (fixes any container collision). The label + item/unit counts stack
        //    compactly on the left.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.unit
            Layout.bottomMargin: Theme.spaceXs
            spacing: Theme.unit
            ColumnLayout {
                spacing: 0
                Text {
                    text: qsTr("Total payable")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                }
                Text {
                    text: qsTr("%1 items · %2 units")
                            .arg(totals.s.itemCount)
                            .arg(Format.qty(totals.s.unitCount))
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                topPadding: Theme.spaceXs
                bottomPadding: Theme.spaceXs
                text: Format.money(totals.s.total)
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontGrand
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideLeft
            }
        }
    }
}
