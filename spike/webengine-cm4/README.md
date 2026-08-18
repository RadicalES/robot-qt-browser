# QtWebEngine CM4 spike (Phase 0)

Throwaway code that answers one question: **does QtWebEngine run acceptably on a
CM4 (2GB RAM, VideoCore VI)?** If the answer is no, the port in issue #6 stops
here and CM4 stays on QtWebKit. Delete this directory once the question is
answered — it is not part of the robot-browser build.

Pi 5 is not in doubt (16GB max, V3D/Mesa, Bookworm arm64). CM4 is the risk.

## What it measures

| Signal | Why it decides go/no-go |
|---|---|
| **Time to `loadFinished`** | Kiosk boots straight into the transaction page. Multi-second startup is user-visible on every reboot. |
| **GL renderer string** | `V3D`/`VideoCore` means the GPU is live. `SwiftShader`/`llvmpipe` means Chromium fell back to software rendering — an immediate no-go on this hardware. |
| **Frame pacing under scroll** | Average FPS *and* worst frame time. A 400ms hitch inside an otherwise smooth average still fails a touch UI. |
| **Peak memory across the process tree** | Chromium forks a zygote, a GPU process and per-site renderers. This is the number that either fits in 2GB or doesn't. |

## Build

### Cross-compile on the workstation (recommended)

```sh
./docker/build-spike-cm4.sh
```

Produces an arm64 binary in `build-spike-cm4/`, built against Qt 6.4.2 +
QtWebEngine from Debian 12's arm64 packages. Verified working.

This uses **CMake, not qmake**: Debian's Qt 6 ships no `linux-aarch64-gnu-g++`
mkspec, so the qmake + `qt.conf` redirect that `docker/build-cm4.sh` uses for
Qt 5 cannot work for Qt 6. See `docker/Dockerfile.cm4-qt6` and
`docker/toolchain-arm64.cmake`.

The device then needs the matching runtime:

```sh
sudo apt install libqt6webenginewidgets6
```

Raspberry Pi OS Bookworm pulls Qt from the Debian repos, so the 6.4.2 the binary
was linked against should match. If it does not, build on-device instead.

### Build on the device

Simpler, and immune to any host/target version skew. Slower to compile, but this
is one source file.

Qt 6 (the stated destination):

```sh
sudo apt install qt6-webengine-dev qt6-base-dev cmake
cmake -B build . && cmake --build build -j4
```

Qt 5.15, only if evaluating the two-step route from issue #6 — the same
`main.cpp` builds against both, and the Qt 5 path additionally sets
`AA_ShareOpenGLContexts`, which Qt 5's QtWebEngine requires:

```sh
sudo apt install qtwebengine5-dev qtbase5-dev
qmake webengine-spike.pro && make -j4
```

`webengine-spike.pro` is kept only for that on-device qmake path;
`CMakeLists.txt` is what the cross-compile uses and builds under either Qt.

## Run

Under the labwc/Wayland session the kiosk actually uses — running it under X11
or a different compositor will not reproduce the real compositing cost:

```sh
export QT_QPA_PLATFORM=wayland
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1

./measure.sh https://the-real-transaction-url/
```

`measure.sh` launches the binary, samples the whole process tree until it
exits, and prints peak/mean memory with the remaining headroom. Use the real
transaction URL — a synthetic test page understates what the actual page costs.

Options passed through to the binary:

- `--scroll <ms>` — scroll-test duration (default 8000)
- `--keep` — stay open after the run, to judge input feel by hand

Run it twice: once cold (straight after boot) and once warm, since the
persistent profile changes startup time on the second run.

## Reading the result

Output lines are prefixed `SPIKE_`:

```
SPIKE_QT 6.4.2
SPIKE_CHROMIUM 102.0.5005.177
SPIKE_LOAD ok 3120 ms from process start
SPIKE_GL V3D 4.2
SPIKE_FPS avg=48.3 worstFrame=112.4ms frames=386
```

Suggested go/no-go thresholds — adjust to what the product actually needs:

- **Memory:** peak under ~1.2GB leaves room for the rest of the system on a 2GB
  CM4. Above ~1.6GB, expect the OOM killer under real use.
- **GL:** anything mentioning SwiftShader or llvmpipe is a fail.
- **Startup:** under ~4s to `loadFinished` on a warm run.
- **Frame pacing:** sustained 30+ FPS with no frame worse than ~150ms.

## Caveats

- Sums **Pss** where `/proc/*/smaps_rollup` is readable, falling back to summed
  RSS, which double-counts Chromium's shared mappings and overstates the total.
  The printed header says which was used.
- Measures the page under *scroll*, which is the common kiosk interaction. It
  does not measure video decode — relevant separately, since Pi 5 dropped the
  hardware H.264 decoder and CM4 did not.
- The virtual keyboard is not exercised here. Qt 6 virtual-keyboard packaging on
  Bookworm is a separate open question in issue #6.
