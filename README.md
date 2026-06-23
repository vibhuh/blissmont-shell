# blissmont-shell

The Qt/C++ POS shell — a thin, keyboard-first, offline-first client that renders the BlissMont
terminal engine. It consumes the **frozen** `terminal.proto` contract and **never** modifies the
engine; the engine is the single source of truth for all cart, tax, tender and sync state.

> Status: **scaffold / walking-skeleton foundation** (Milestone 1). The full project structure,
> build system, library-first architecture, and the skeleton's classes + QML are in place. It is
> designed to build on a provisioned dev machine (Qt 6.8 + vcpkg); it has not been compiled in the
> authoring environment (no Qt/vcpkg there). See **Status & next steps**.

## Architecture (MVVM, unidirectional — single source of truth = the engine)

```
QML (View) → ViewModel (intent + view-only state) → Bridge (command) → engine
QML binding ← ViewModel ← Model/Summary/signal ← Bridge (event) ← ────────┘
```

- **`PosEngineBridge`** (`src/bridge/`) — the only thing that touches gRPC. Owns the localhost
  bidi `Session` stream; a dedicated reader thread marshals engine Events to the UI thread via
  `Qt::QueuedConnection`. UI→engine are thin `Q_INVOKABLE` commands; engine→UI is `CartLineModel`
  (full reset per snapshot) + `CartSummary` Q_PROPERTYs + discrete Qt signals.
- **Models** (`src/models/`) — `CartLineModel`, `CartSummary`. Reflect snapshots verbatim; never
  do pricing/tax math.
- **ViewModels** (`src/viewmodels/`) — per-screen intent only (scan text, status, selections).
  Never duplicate engine state. Unit-tested headless.
- **Services** (`src/services/`) — `ConnectionService` (connectivity/sync status), `ConfigService`
  (resolved feature flags — see the integration gap below).
- **`core/`** — Qt-free value types (`Money`, `Result`). Mirrors the engine's exact
  decimal/minor-unit money rules; the most heavily unit-tested layer.

**Library-first (the testability keystone):** `core + bridge + models + viewmodels + services`
compile into one static `libshell`. The GUI app links it; the unit tests link it. Logic is tested
with no display and no QML.

## Toolchain

| Need | Version | Install |
|---|---|---|
| Qt | **6.8 LTS** | Qt online installer / `aqt` / distro — separate from vcpkg |
| C++ | **C++20** | g++ ≥ 12 or clang ≥ 15 |
| CMake | ≥ 3.21 | distro / Kitware |
| protobuf, gRPC, GoogleTest | pinned | **vcpkg manifest** (`vcpkg.json`) |

## Build

```bash
export VCPKG_ROOT=/path/to/vcpkg            # used by CMakePresets.json
cmake --preset linux-debug \
      -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.0/gcc_64   # point at your Qt 6.8
cmake --build --preset linux-debug
ctest --preset linux-debug                  # runs the headless unit suite
./build/linux-debug/src/blissmont-shell     # launches the shell (needs a running engine)
```

First configure compiles protobuf/grpc/gtest from source via vcpkg (slow once, cached after).
To **pin** the dependency versions, run `vcpkg x-update-baseline --add-initial-baseline` in this
repo to stamp a `builtin-baseline` into `vcpkg.json`.

## Contract codegen — consumed, never edited (spec §3)

`terminal.proto` is canonical in **blissmont-contracts**, vendored here as a git submodule at
`third_party/blissmont-contracts/` (pinned `v1.0.1`). `cmake/ProtoGen.cmake` runs `protoc` +
`grpc_cpp_plugin` at build time into `src/proto/` (gitignored, regenerated fresh) so the shell's
stubs can never drift from the engine's contract. A contract change is fixed in
blissmont-contracts first, the submodule bumped, and codegen re-run — the generated stubs are
never hand-patched.

To bump the contract:

```bash
git -C third_party/blissmont-contracts fetch --tags
git -C third_party/blissmont-contracts checkout v1.x.y
git add third_party/blissmont-contracts && git commit -m "bump contracts to v1.x.y"
```

## The walking skeleton (spec §6)

The thinnest end-to-end slice that proves the architecture: app boots → bridge connects →
billing screen renders (empty `BillTable`, focused scan field, status line) → **type an item code
→ `scanItem` → engine → `CartUpdated` → model updates → line appears, focus returns to scan**.
That single loop validates connect/command/receive/render. `tests/integration/BridgeSmokeTest`
automates it against a live engine (`BLISSMONT_ENGINE_TARGET=localhost:50080`).

## Known integration gap — config relay (a validated UI gap, spec §3)

`ConfigService` needs the resolved terminal config (which panels/actions are enabled). But
`TerminalConfigSnapshot`/`GetTerminalConfig` live in `blissmont.pos.v1` (the terminal↔**server**
contract), **not** in `blissmont.terminal.v1` (the UI↔**engine** contract the shell consumes). The
shell talks only to the local engine, so it has no path to those flags today. Closing this is a
**contract change** in blissmont-contracts — preferably the engine relaying the resolved config to
the UI over the `Session` stream (a new `ConfigSnapshot` event in `terminal.proto`) — then a
submodule bump + regen. Until then `ConfigService` serves safe defaults so the skeleton runs.

## Decisions (spec §7 forks)

- **Proto vendoring:** git submodule of **blissmont-contracts** (single source of truth).
  *(Corrects the spec's pre-Milestone-0 wording that named rachis-core — `terminal.proto` was
  moved into blissmont-contracts in Milestone 0.)*
- **C++ deps:** **vcpkg** manifest (pinned, reproducible).
- **Qt licensing:** unresolved business decision (commercial vs open-branch self-build for 5 yrs
  of 6.8 patches) — not blocking the scaffold.

## Status & next steps

- ✅ Repo structure, CMake + presets, `ProtoGen.cmake`, `libshell` (core/bridge/models/viewmodels/
  services), QML skeleton (billing screen + components + theme), unit + integration test suites.
- ⏭️ On a provisioned machine: install Qt 6.8 + bootstrap vcpkg, `cmake --preset linux-debug`,
  run the unit suite, then the live `scanItem` round-trip against the engine.
- ⏭️ Close the config-relay contract gap, then fill in panels (tender, return, history, payout)
  against the proven architecture.
