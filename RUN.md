# RUN.md — building and running BlissMont POS locally (Ubuntu 24.04)

The repeatable procedure to stand up the **walking skeleton** (Milestone 1) on a fresh
Ubuntu 24.04 machine: install the toolchain, build the Qt shell, run the **engine**
(rachis-core) and the **shell** together, and see the billing screen hydrate + a scan
round-trip through real gRPC.

> The shell is a **client**. It renders nothing useful without the engine running on
> localhost — both processes must be up to see the round-trip. The engine is the single
> source of truth for all cart/tax/tender/sync state.

Verified on: Ubuntu 24.04, g++ 13.3, CMake 3.28, Ninja 1.11, Go 1.26, Qt **6.8.3**.

---

## 1. Prerequisites

### 1.1 Build tools (apt)

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ninja-build pkg-config \
  libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev
```

`g++` (≥ 12, for C++20), `cmake` (≥ 3.21) and `git` are also required. On a stock 24.04
desktop the GL/EGL/xkbcommon dev packages are often already present (`libgl-dev`,
`libegl1-mesa-dev`, `libxkbcommon-dev`); install only what `apt` reports missing.

### 1.2 Qt 6.8.3 via aqtinstall (account-free, matches CI)

CI pins `6.8.*` (`jurplel/install-qt-action@v4`), which resolves to the **latest 6.8
patch = 6.8.3**. Install that exact version with [aqtinstall] so it matches CI; do **not**
use Ubuntu's apt Qt (24.04 ships 6.4.x — wrong version).

```bash
pipx install aqtinstall            # or: pip install --user aqtinstall
export PATH="$HOME/.local/bin:$PATH"

# Install Qt 6.8.3 desktop gcc_64 to ~/Qt.
# IMPORTANT: pin -b to a fast mirror (see Troubleshooting — the default geo-redirect
# stalls). ftp.fau.de mirrors the full Qt repo and is reliable.
aqt install-qt linux desktop 6.8.3 linux_gcc_64 \
  -b https://ftp.fau.de/qtproject/ \
  --outputdir "$HOME/Qt"
```

This yields `~/Qt/6.8.3/gcc_64` with the modules the shell needs: Core, Gui, Qml, Quick,
QuickControls2, Test.

[aqtinstall]: https://github.com/miurahr/aqtinstall

### 1.3 vcpkg (protobuf / gRPC / GoogleTest)

`CMakePresets.json` wires the vcpkg toolchain via `$env{VCPKG_ROOT}`, and `vcpkg.json`
pins the C++ deps in manifest mode.

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/vcpkg"     # consumed by the preset
```

### 1.4 The contracts submodule

`terminal.proto` is vendored as the `blissmont-contracts` submodule (currently pinned at
**v1.1.0**). If you cloned without `--recursive`:

```bash
git submodule update --init --recursive
test -f third_party/blissmont-contracts/proto/terminal/v1/terminal.proto && echo OK
```

---

## 2. Build the shell

```bash
export VCPKG_ROOT="$HOME/vcpkg"

# Configure. CMAKE_PREFIX_PATH points CMake at the aqt Qt; the preset adds the vcpkg
# toolchain. Drop the -D if Qt is already on CMAKE_PREFIX_PATH.
cmake --preset linux-debug -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.3/gcc_64"

# Build (GUI app + libshell + unit/integration test binaries).
cmake --build --preset linux-debug
```

**First configure is slow.** vcpkg compiles protobuf, gRPC, abseil and GoogleTest from
source — in **both** debug and release — which is ~30–60 min on 8 cores. This happens once:
binaries are cached under `~/.cache/vcpkg/archives`, so a clean reconfigure in a fresh
`build/` restores them in seconds, and an incremental rebuild skips them entirely. (CI
caches the same `~/.cache/vcpkg/archives` keyed on `vcpkg.json`.)

`terminal.proto` C++/gRPC stubs are generated fresh into `src/proto/` (gitignored) on every
build by `cmake/ProtoGen.cmake` — never edited, never committed.

### Unit suite (headless — no engine, no display)

```bash
ctest --preset linux-debug          # or: ctest --test-dir build/linux-debug
```

Covers `Money`, `CartLineModel`, `BillingViewModel`, `ConfigService`. The integration
tests (`shell_integration_tests`) **self-skip** unless `BLISSMONT_ENGINE_TARGET` is set
(see §4), so the suite stays green on a box with no engine.

---

## 3. Run the engine (rachis-core)

The shell talks only to the local engine over the `terminal.proto` `Session` bidi stream.
`rachis-core` ships a local-run entrypoint that serves that stream over TCP, booting the
engine against an in-memory fake POS (config + product cache — no DB, no backend, no
network required):

```bash
cd /path/to/rachis-core
go run ./cmd/blissmont-engine          # listens on 127.0.0.1:50080
```

Useful flags / env:

| Flag | Env | Default | Purpose |
|---|---|---|---|
| `-addr` | `BLISSMONT_ENGINE_ADDR` | `127.0.0.1:50080` | TCP address for the Session stream |
| `-db`   | `BLISSMONT_ENGINE_DB`   | ephemeral temp file | persistent engine SQLite store |

On boot it logs the listen address and seeds three demo products — barcode **`TESTSKU`**
(used by the shell smoke test) plus **`111`** (Widget) and **`222`** (Gadget) for manual
sessions:

```
INFO engine booted against fake POS products=3
INFO TerminalEngine.Session listening — point the shell here addr=127.0.0.1:50080 ...
```

Leave it running. Stop with Ctrl-C (graceful shutdown).

> `cmd/blissmont-harness` is a *different* tool — it drives a full scripted shift over an
> in-process bufconn and exits. Use `cmd/blissmont-engine` for an interactive shell.

---

## 4. Run the shell against the engine

In a second terminal:

```bash
./build/linux-debug/src/blissmont-shell
```

The shell **auto-connects to `localhost:50080`** on startup (`Main.qml`'s
`Component.onCompleted: PosEngineBridge.connectToEngine()` — the default target is baked
into `PosEngineBridge::connectToEngine`). No flag is needed when the engine is on the
default port. To target a different address, change that default (it is the single source
of the engine target) and rebuild.

### Automated round-trip proof (headless, no display)

The same loop is asserted by the integration smoke test against a live engine:

```bash
# with cmd/blissmont-engine running on :50080
BLISSMONT_ENGINE_TARGET=localhost:50080 \
  ctest --test-dir build/linux-debug -R BridgeSmoke --output-on-failure
# or run the binary directly:
BLISSMONT_ENGINE_TARGET=localhost:50080 ./build/linux-debug/tests/shell_integration_tests
```

`BridgeSmokeTest` connects the real bridge, scans `TESTSKU`, and asserts a `CartUpdated`
round-trips and resets the model — the spec §6 walking-skeleton proof.

---

## 5. What a successful run looks like

- **Billing screen renders**: empty bill table, a focused scan field ("scan-is-home"), a
  status line.
- **Config hydrated from the engine**: on connect the engine pushes `ConfigUpdated`, so the
  shell shows the store identity from the fake config (store **BlissMont Demo**, currency
  **Rs**, returns/payouts enabled). The engine logs the connection; the shell's status line
  goes connected.
- **Scan round-trips**: type `111` + Enter in the scan field → `scanItem` → engine →
  `CartUpdated` → a **Widget** line appears in the bill table and focus returns to the scan
  field. (`222` → Gadget; an unknown code → "Item not found" in the status line.)

That single connect → command → event → render loop is the whole point of Milestone 1.
Tender / returns / history / payout panels are intentionally still placeholders.

---

## 6. Troubleshooting

**`aqt` download stalls / crawls (the big one).** `download.qt.io` geo-redirects archive
fetches to a nearby mirror, and some mirrors stall or throttle to a few KB/s mid-download
(`qtbase` is the usual victim). Symptoms: `~/Qt` stops growing, archives in `/tmp/tmp*/`
sit at a few hundred KB. Fix: pin `-b https://ftp.fau.de/qtproject/` (or another reliable
full mirror). If a single file is throttled, kill aqt (`pkill -9 -f 'aqt install-qt'`),
remove the partial (`rm -rf ~/Qt/6.8.3 /tmp/tmp*/`) and re-run — a fresh stream is usually
fast.

**CMake can't find Qt6 / `qmake` not found.** Pass `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/gcc_64`
to the configure step (or export `CMAKE_PREFIX_PATH`). The patch version must be `6.8.3`.

**`VCPKG_ROOT` unset → toolchain file not found.** `export VCPKG_ROOT=$HOME/vcpkg` before
configuring; the preset references `$env{VCPKG_ROOT}`.

**vcpkg rebuilds deps from scratch every time.** Make sure `~/.cache/vcpkg/archives`
persists between runs; deleting it (or changing `vcpkg.json`) forces the ~1h rebuild.

**Qt fails to load the platform plugin / no GUI.** Install `libgl1-mesa-dev`
`libegl1-mesa-dev` `libxkbcommon-dev`, and run on a machine with a display
(`echo $DISPLAY`). For a headless box, use the integration smoke test (§4) instead of the
GUI — it needs no display.

**Shell connects but the bill stays empty / "Item not found".** Scan one of the seeded
barcodes (`TESTSKU`, `111`, `222`). The fake catalog only knows those.

**`listen 127.0.0.1:50080: address already in use`.** A previous engine is still running
(`pgrep -f blissmont-engine`) — stop it, or start the new one on another port with
`-addr 127.0.0.1:50081` and point the shell there.

**Shell can't reach the engine.** The engine defaults to `127.0.0.1`; the shell dials
`localhost`. If your `localhost` resolves to IPv6 `::1` first and the connection fails,
start the engine with `-addr :50080` (all interfaces) or `-addr localhost:50080`.
