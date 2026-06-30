# BlissMont Shell — Core Cash-Sale Loop + Tasks Launcher (CC brief)

_The sale screen renders and the engine commits a bill (Charge→Tender→Settle is wired), but the
action bar has stubs and a completed sale shows no confirmation. This brief makes the CORE CASH
SALE visibly complete end-to-end, finalizes the action bar to business verbs, and adds the Tasks
launcher. Shell/QML + ViewModel — build directly, review the running result. No engine/contract/
accounting change (all the engine paths already exist; this is wiring + UI)._

## Two design rules this brief establishes (apply app-wide)
1. **Bottom bar = "what can I do with the current bill?"** — sale actions only. Operational
   actions (terminal/drawer/business) live under Tasks. Test: belongs to the sale → bar; doesn't
   → Tasks. Never let Tasks become a dumping ground.
2. **Menus choose, panels work.** Tasks is a launcher: pick an item → menu closes → the
   right-side panel (or screen) for that workflow opens. One interaction model everywhere.

---

## 1. Verify the core sale commits (empirical, first)
Confirm Charge (F12) → Tender → Settle actually commits a bill end-to-end against the running
engine: ring items, charge, tender to zero balance, settle. Confirm the engine records the
order. Report any dead-end. (CC's trace says this path is wired via `TenderViewModel::complete`
→ `bridge_->settle()`; verify it actually completes a sale, don't just re-read the code.)

## 2. Wire `orderSettled` — the post-settle confirmation (HIGHEST VALUE)
Today nothing in QML consumes `orderSettled` — a cashier settles and sees nothing. Wire the
bridge's `orderSettled(receiptNo, provisional, total)` signal to a confirmation sequence that
fires within ~1s of settle:
- A clear **"Sale completed"** confirmation (green/success treatment).
- **Invoice / receipt number** shown prominently.
- **Amount received** and **change due** (if any) — from the tender just taken.
- **Print** per config (`default_receipt` = print|no_print) — automatic receipt at settle.
- **Reset the screen for the next sale** — clear the cart, return the right panel to its
  product-search home state, focus the scan field. The cashier should never wonder whether the
  sale completed, and should be ready to ring the next customer immediately.
This is the single most important fix — it's what makes the core sale *feel* complete.

## 3. Wire Print → `ReprintBill`
The Print button is a stub. Wire it to the existing `ReprintBill` path (already used by the
History panel's DUPLICATE reprint) against the just-settled receipt #. Mechanical — the engine
path exists.

## 4. Remove "Save"; finalize the action bar
Remove the "Save" button and its `"separate build"` stub entirely (business verbs, not software
verbs — "Save" is ambiguous in a POS). Final bottom bar, uniform icon buttons (per the earlier
spec — identical size, tooltips, keyboard shortcuts, state-gated enable/disable):

`Charge (F12) · Hold · Discount · Sundry · Print · Void · Tasks ▾`

- **Charge** — wired (navState=tender). **Hold** — wired (`holdCart`). **Void** — wired
  (`voidCart`). **Discount** — bill-level discount panel (per the frozen spec). **Sundry** —
  add a manual non-product charge line (delivery/packing/manual charge); pairs with Discount
  (Discount reduces the bill, Sundry adds to it). The sundry line is `discount_eligible=false`
  (per ADR-0009 discount-eligibility — a sundry charge is not discountable). **Print** — §3
  above. **Tasks ▾** — §5; the ▾ indicates it opens a menu, not an immediate action.
- Keep "Sundry" as the label (familiar Indian-billing term, pairs with Discount).

## 5. Tasks launcher (popup menu → panels)
A lightweight popup menu (NOT a modal, NOT a workspace). Selecting an item closes the menu and
opens that workflow's right-side panel or screen. Flat list for now (group into sections later
when it grows past ~15 items — Transactions / Cash Drawer / Reports / Utilities / Administration
— but flat is correct today):

```
History       → History panel (built)
Payout        → Payout panel (built)
Cash In       → Cash In panel (engine posts cash_in; wire/confirm the panel)
Open Drawer   → escpos drawer-pulse (confirm command exists; immediate action)
X Report      → management/EOD-batch report surface (server-side built)
Z Report      → till-print Z-report (built; triggers the device print)
Calculator    → small local calculator widget (new, trivial — local only)
Settings      → company-config admin (server API built; basic settings surface)
```
Most items wire EXISTING capabilities into the launcher — report what's already reachable vs.
what needs a small new panel (likely just Calculator, and possibly a thin Settings surface).
Don't build new engine paths; surface what exists.

## Out of scope (named, deferred — not dropped)
- **On Account / Credit Sale (khata)** — deferred until the cash loop is polished. It's a full
  AR slice (registered-customer requirement, `Dr AR / Cr Sales / Cr GST` per-invoice per
  ADR-0009, credit validation, statements, collections) and lands cleaner as its own designed
  slice after this. Not on the bar yet.
- Decimal quantity entry, keyboard-first lookup, the full UX polish pass — separate briefs after
  the core loop is confirmed working.

## Acceptance — VERIFIED 2026-06-29 (PR vibhuh/blissmont-shell#11, commit bc2115e)
- [x] **Cash sale completes end-to-end with visible confirmation (invoice #, change due), prints per
  config, resets for the next sale — within ~1s of settle.** Scan→tender→settle→`OrderSettled`
  proven over real gRPC against the dev engine (`shell_integration_tests`, ~tens of ms). Receipt
  prints at settle — **engine-owned** (`settle.Settle`; engine log shows `printed ESC/POS receipt`).
  `SaleCompleteOverlay` shows invoice #/received/change (reads the pre-reset summary in the
  `orderSettled` handler), then resets to product-search home + re-focuses scan. Confirmation/reset
  verified by code + an offscreen runtime probe (overlay + every panel instantiate, zero QML
  errors); NOT visually captured on a live settle (no input-injection tool on this box).
- [x] **Print reprints the settled bill.** Wired to `reprintBill(lastReceiptNo)`; the reprint path
  is proven by `BridgeSmoke.HistoryRecallReprintRoundTrip`.
- [x] **"Save" is gone; the action bar is the final 7-button set, uniform, with Tasks ▾.** Confirmed
  by X11 screenshot: `Charge · Hold · Discount · Sundry · Print · Void · Tasks ☰`, uniform, Save absent.
- [x] **Tasks opens a popup; each item launches its panel/screen; menu closes on selection.** Built
  (`TasksMenu`; Menu closes on select by default); all target panels instantiate cleanly (probe).
  History/Payout/Cash In/Calculator/Settings/Z Report reachable; Open Drawer + X Report surfaced as
  honest notices (no terminal-contract command). Not click-driven (no input injection).
- [x] **No regression to Charge/Hold/Void/tender; build + shell tests green.** 68 unit + 7 integration
  green (incl. Tender add/remove, return, suspend/resume, history round-trips + new
  `CashInRecordsAndEchoes`); 1 skipped by design (`refund_tender_mode=both`). Build clean.
