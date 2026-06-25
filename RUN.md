# RUN.md — building and running BlissMont POS locally (Ubuntu 24.04)

The repeatable procedure to stand up the **walking skeleton** (Milestone 1) on a fresh
Ubuntu 24.04 machine: install the toolchain, build the Qt shell, run the **engine**
(rachis-core) and the **shell** together, and see the billing screen hydrate + a scan
round-trip through real gRPC.

> The shell is a **client**. It renders nothing useful without the engine running on
> localhost — both processes must be up to see the round-trip. The engine is the single
> source of truth for all cart/tax/tender/sync state.

Verified on: Ubuntu 24.04, g++ 13.3, CMake 3.28, Ninja 1.11, Go 1.26, Qt **6.8.3** (8-core /
16 GB laptop). End to end: 15/15 unit tests pass, both integration tests pass against the
live engine, and the GUI renders + hydrates config + scans a line over real gRPC.

---

## 1. Prerequisites

### 1.1 Build tools (apt)

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ninja-build pkg-config \
  libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev \
  libxcb-cursor0          # REQUIRED to launch the GUI on X11 (Qt ≥ 6.5 xcb plugin)
```

`g++` (≥ 12, for C++20), `cmake` (≥ 3.21) and `git` are also required. On a stock 24.04
desktop the GL/EGL/xkbcommon dev packages are often already present (`libgl-dev`,
`libegl1-mesa-dev`, `libxkbcommon-dev`); install only what `apt` reports missing.

> **`libxcb-cursor0` is easy to miss** and CI doesn't need it (CI never opens a window). Without
> it the GUI aborts at startup with *"Could not load the Qt platform plugin xcb"*. See
> Troubleshooting for a no-display capture option that sidesteps X11 entirely.

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

**First configure is very slow.** vcpkg compiles abseil, protobuf, gRPC, openssl and
GoogleTest from source — in **both** debug and release. On this 8-core / 16 GB laptop the
first `cmake --preset` took **~105 min** (`Configuring done (6361.7s)`), gRPC 1.76 alone
dominating (its giant generated TUs swap-pressure a 16 GB box). Budget 1.5–2 h the first
time; on more RAM / cores it's closer to 30–45 min. This happens **once**: binaries are
cached under `~/.cache/vcpkg/archives`, so a clean reconfigure in a fresh `build/` restores
them in seconds and an incremental rebuild skips them entirely. (CI caches the same
`~/.cache/vcpkg/archives` keyed on `vcpkg.json`.) The actual project build after configure
(`cmake --build`) is fast — ~1–2 min (77 targets).

`terminal.proto` C++/gRPC stubs are generated fresh into `src/proto/` (gitignored) on every
build by `cmake/ProtoGen.cmake` — never edited, never committed.

### Unit suite (headless — no engine, no display)

```bash
ctest --preset linux-debug          # or: ctest --test-dir build/linux-debug
```

Covers `Money`, `CartLineModel`, `BillingViewModel`, `ConfigService` — **15 tests, all
pass**. The integration tests (`shell_integration_tests`) **self-skip** unless
`BLISSMONT_ENGINE_TARGET` is set (see §4), so the suite stays green on a box with no engine.

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
round-trips and resets the model — the spec §6 walking-skeleton proof. Verified: both
`BridgeSmoke.ScanRoundTripsAndRenders` and `ConfigRehydrate.ConfigRepopulatesAfterReconnect`
pass against the live engine (`2 tests … PASSED`, ~25 ms).

> Run the integration binary itself with `QT_QPA_PLATFORM=offscreen` if the box has no
> display — it needs no window, only the engine.

---

## 5. What a successful run looks like

- **Billing screen renders**: empty bill table, a focused scan field ("scan-is-home"), a
  status line.
- **Config hydrated from the engine**: on connect the engine pushes `ConfigUpdated`, so the
  shell shows the store identity from the fake config (store **BlissMont Demo**, currency
  **Rs**, returns/payouts enabled). The engine logs the connection; the shell's status line
  goes connected.
- **Scan round-trips**: type `111` + Enter in the scan field → `scanItem` → engine →
  `CartUpdated` → a **Widget** line appears in the bill table — qty `1`, price `100.00`,
  line total `100.00`, and the running **Total** updates to **118.00** (100 + 18% GST) — and
  the scan field clears and re-focuses ("scan-is-home"). (`222` → Gadget; an unknown code →
  "Item not found" in the status line.)

This was captured live: the billing screen renders with the focused scan field, empty bill
("Scan an item to begin"), Walk-in customer, Total 0.00, status line, and the item-panel
placeholder; after scanning `111` the Widget line and Total 118.00 appear. That single
connect → command → event → render loop is the whole point of Milestone 1. Tender / returns
/ history / payout panels are intentionally still placeholders.

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

**`Could not load the Qt platform plugin "xcb"` → app aborts at startup.** The xcb plugin
"was found" but a dependency is missing — almost always `libxcb-cursor0` (mandatory since
Qt 6.5). `sudo apt-get install -y libxcb-cursor0`. Also ensure `libgl1-mesa-dev`
`libegl1-mesa-dev` `libxkbcommon-dev` and a display (`echo $DISPLAY`).

> **No sudo? Install `libxcb-cursor0` into a local dir** (its deps — `libxcb-render0`,
> `libxcb-image0`, `libxcb1`, … — ship with a stock 24.04 desktop, so only the cursor lib is
> missing). `apt-get download` needs no root:
>
> ```bash
> apt-get download libxcb-cursor0                      # → libxcb-cursor0_*.deb in CWD
> dpkg-deb -x libxcb-cursor0_*.deb ./xcbcursor          # extract, no install
> LD_LIBRARY_PATH="$PWD/xcbcursor/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH" \
>   ./build/linux-debug/src/blissmont-shell             # launches on $DISPLAY
> ```
>
> This is per-launch (the env var), not a system install — verified working on this box. A
> proper `sudo apt-get install -y libxcb-cursor0` lets you drop the `LD_LIBRARY_PATH`.

**No display at all (SSH / headless) but you still want a screenshot.** Run the GUI on Qt's
self-contained VNC platform — it needs none of the X11 libs above:

```bash
QT_QPA_PLATFORM='vnc:size=1280x800' ./build/linux-debug/src/blissmont-shell   # serves :5900
# then drive + capture with any VNC client, e.g. vncdotool (pip install vncdotool):
vncdotool -s localhost::5900 capture before.png
vncdotool -s localhost::5900 type 111 key enter
vncdotool -s localhost::5900 capture after.png      # Widget line + Total 118.00
```

This is exactly how the screenshots in §5 were captured headlessly. For a pure pass/fail
proof with no rendering at all, use the integration smoke test (§4).

**Shell connects but the bill stays empty / "Item not found".** Scan one of the seeded
barcodes (`TESTSKU`, `111`, `222`). The fake catalog only knows those.

**`listen 127.0.0.1:50080: address already in use`.** A previous engine is still running
(`pgrep -f blissmont-engine`) — stop it, or start the new one on another port with
`-addr 127.0.0.1:50081` and point the shell there.

**Shell can't reach the engine.** The engine defaults to `127.0.0.1`; the shell dials
`localhost`. If your `localhost` resolves to IPv6 `::1` first and the connection fails,
start the engine with `-addr :50080` (all interfaces) or `-addr localhost:50080`.
