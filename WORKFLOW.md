# Development Workflow

## Branch Strategy

| Branch | Purpose | Deploys to |
|---|---|---|
| `dev` | Active development | Developer machines (local builds) |
| `beta` | Testing on target hardware | Test devices |
| `master` | Production releases | Production devices |

```
dev ──────► beta ──────► master
 (develop)   (test)      (release)
```

- All work happens on `dev`
- When ready for hardware testing, merge `dev` → `beta`
- After successful testing, merge `beta` → `master`

```sh
# Promote to beta for testing
git checkout beta
git merge dev
git push origin beta

# Promote to production after testing
git checkout master
git merge beta
git push origin master

# Return to dev
git checkout dev
```

## Local Development

### Prerequisites

Qt 5.15 with modules: core, gui, widgets, network, quickwidgets, quickcontrols2, virtualkeyboard, websockets, dbus, and QtWebKit 5.212.

On Debian/Ubuntu:

```sh
sudo apt-get install -y \
    qtbase5-dev qtdeclarative5-dev qtquickcontrols2-5-dev \
    libqt5websockets5-dev libqt5webkit5-dev libqt5virtualkeyboard5-dev \
    qtvirtualkeyboard-plugin qt5-qmake qtbase5-dev-tools
```

### Build and run

```sh
cd src
qmake
make -j$(nproc)
./robot-browser http://example.com
```

After editing source files, just run `make` again (no need to re-run `qmake` unless `.pro` or `.qrc` files changed).

### Clean build

```sh
cd src
make clean        # remove object files
make distclean    # remove everything including Makefile
qmake && make     # full rebuild
```

## Cross-Compilation

Cross-compilation uses Docker containers with Debian 12 and the target architecture's Qt development packages. Docker must be installed on the build host.

### BeagleBone Black (armhf)

```sh
./docker/build-bbb.sh
```

- Docker image: `rbrowser-bbb-build` (Debian 12 + `crossbuild-essential-armhf`)
- qmake spec: `linux-arm-gnueabi-g++` with explicit `QMAKE_CC`/`QMAKE_CXX` overrides for `arm-linux-gnueabihf-g++`
- Output: `build-bbb/robot-browser` (ELF 32-bit ARM)

### Raspberry Pi CM4 (arm64)

```sh
./docker/build-cm4.sh
```

- Docker image: `rbrowser-cm4-build` (Debian 12 + `crossbuild-essential-arm64`)
- qmake spec: `linux-aarch64-gnu-g++`
- Output: `build-cm4/robot-browser` (ELF 64-bit ARM aarch64)

### Build output

Both `build-bbb/` and `build-cm4/` are gitignored. Docker caches the build image after the first run, so subsequent builds only recompile changed source files.

To force a clean cross-build, delete the build directory:

```sh
rm -rf build-bbb && ./docker/build-bbb.sh
rm -rf build-cm4 && ./docker/build-cm4.sh
```

## Debian Packaging

Build `.deb` packages for any supported architecture:

```sh
./scripts/build-deb.sh arm64    # CM4 — Docker cross-compile + package
./scripts/build-deb.sh armhf    # BBB — Docker cross-compile + package
./scripts/build-deb.sh amd64    # PC  — local build + package
```

Output: `build-deb/robot-browser_2.1.0-1_<arch>.deb`

### Package contents

| Path | Description |
|---|---|
| `/usr/bin/robot-browser` | Application binary |
| `/usr/lib/robot-browser/robotbrowser.sh` | Startup wrapper (auto-detects display server) |
| `/usr/lib/systemd/system/robot-browser.service` | Systemd service unit |
| `/etc/robot-browser/appconfig` | Configuration (preserved on upgrade) |
| `/etc/udev/rules.d/99-robot-input.rules` | Touchscreen/keyboard device symlinks |

### Configuration

Edit `/etc/robot-browser/appconfig` on the target:

```sh
WB_LOAD_URL=http://192.168.100.1/transaction
WB_LAYOUT=portrait    # or landscape
```

### Publishing to package repository

Copy the `.deb` to the package pool and regenerate metadata:

```sh
cp build-deb/robot-browser_2.1.0-1_arm64.deb /path/to/package-repository/debian/pool/main/arm64/
cp build-deb/robot-browser_2.1.0-1_armhf.deb /path/to/package-repository/debian/pool/main/armhf/
cp build-deb/robot-browser_2.1.0-1_amd64.deb /path/to/package-repository/debian/pool/main/amd64/
cd /path/to/package-repository && ./scripts/update-debian-repo.sh
```

## Deploying to Devices

### Via apt (recommended)

On a device with the Radical ES repo configured:

```sh
sudo apt-get update
sudo apt-get install robot-browser
sudo systemctl start robot-browser
```

### Manual deployment

Copy the binary directly and restart:

```sh
# BBB
scp build-bbb/robot-browser root@<bbb-ip>:/usr/bin/robot-browser
ssh root@<bbb-ip> "systemctl restart robot-browser"

# CM4
scp build-cm4/robot-browser root@<cm4-ip>:/usr/bin/robot-browser
ssh root@<cm4-ip> "systemctl restart robot-browser"
```

For first-time manual setup, see [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md).

## Project Files

| File | Re-run `qmake` after editing? |
|---|---|
| `src/*.cpp`, `src/*.h` | No — just `make` |
| `src/qml/*.qml` | No — just `make` (embedded in QRC) |
| `src/js/*.js` | No — just `make` (embedded in QRC) |
| `src/robot-browser.pro` | Yes |
| `src/robot-browser.qrc` | Yes |

## Testing Browser Compatibility

Load the feature test page to verify JS/CSS polyfill support on a target device:

```sh
./robot-browser file:///path/to/docs/feature-test.html
```

See [BROWSER-COMPATIBILITY.md](BROWSER-COMPATIBILITY.md) for expected results.
