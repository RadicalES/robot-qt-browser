# Development Workflow

Daily development and release workflow for robot-browser. The branching and
release model is the same one the Robot SCADA Server uses, so the two projects
promote and release the same way.

## Branching Strategy

```
dev  →  beta  →  main
```

| Branch | Purpose |
|--------|---------|
| `dev` | Active development — all work happens here |
| `beta` | Pre-release testing on target hardware. Also used for hotfixes to main |
| `main` | Production — the code running on live terminals. Only merges, no direct development |

**Packages published to the CDN are built from `main`.** A `.deb` on
`packages.radicales.net` is what terminals install by `apt-get install`, so it
must come from the released branch. Test builds from `dev` or `beta` go onto a
device by hand (see [Manual install](#manual-install)), never through the
repository.

## Version Numbering

Single source of truth: the `VERSION` file in the project root, injected at
compile time as `APP_VERSION` and read by `scripts/build-deb.sh`.

Version format: `MAJOR.MINOR.PATCH` (e.g. `3.0.1`)

- **PATCH** — incremented on every commit to `dev` or hotfix on `beta`
- **MINOR** — bumped on `dev` after every release to main, so dev/beta versions
  never clash with main
- **MAJOR** — breaking changes (3.0.0 was the QtWebKit → QtWebEngine port)

```sh
./scripts/bump-version.sh patch     # 3.0.1 → 3.0.2
./scripts/bump-version.sh minor     # 3.0.1 → 3.1.0
./scripts/bump-version.sh 4.0.0     # set explicitly
```

### Tag format

Tags are branch-prefixed so pre-release tags cannot collide with releases:

| Branch | Tag format | Example |
|--------|-----------|---------|
| `dev` | `dev-vX.Y.Z` | `dev-v3.1.0` |
| `beta` | `beta-vX.Y.Z` | `beta-v3.0.2` |
| `main` | `vX.Y.Z` | `v3.0.2` |

Pushing a tag triggers a GitHub Action that creates a release. Tags that are
not plain `vX.Y.Z` are marked pre-release.

## Development

All work is done on `dev` (or feature branches off `dev`).

1. Make changes in `src/`
2. Bump the patch version
3. Build and test — on hardware if the change touches the session, the
   keyboard, or anything the framebuffer or compositor can break
4. Commit and push to `dev`

```sh
git checkout dev
# ... make changes ...
./scripts/bump-version.sh patch
./docker/build-cm4.sh                # verify it compiles for the target
git add ... && git commit
git push origin dev
```

## Releasing

### 1. Promote dev to beta

```sh
git checkout beta && git merge dev && git push origin beta
```

### 2. Test on beta

Install the beta build on a test terminal by hand and exercise it. If issues
are found, fix on `beta` directly, bump the patch version, commit and push.

### 3. Promote beta to main

```sh
git checkout main && git merge beta && git push origin main
```

### 4. Tag and release from main

```sh
./release.sh          # --dry-run to preview
./push-release.sh
```

`release.sh` drafts a `RELEASE.md` entry from the commits since the last tag,
opens your editor to refine it, then commits and tags. `push-release.sh` pushes
the branch and the tag, which triggers the GitHub Release.

Add a `debian/changelog` entry for the version in the same commit — it is what
`apt changelog` shows on the terminal.

### 5. Publish the package to the CDN

From `main`, with the release tagged:

```sh
./scripts/publish-deb.sh arm64        # --dry-run to preview
```

That builds the package if needed, copies it into the package repository's
pool, removes the version it replaces, and hands off to that repository to
regenerate the signed metadata, sync to R2 and verify. Everything downstream of
the pool belongs to the package repository, so nothing here touches rclone.

It refuses to publish from `dev` or `beta`, with an unclean tree, or for an
untagged version — a package on the CDN is what terminals install by apt.

Set `PACKAGE_REPO` if the package repository is not at
`/home/janz/data/package-repository`.

Verify from a terminal:

```sh
sudo apt-get update && apt-cache policy robot-browser
```

If apt still offers the old version, Cloudflare is serving a cached
`dists/.../Packages`; purge it from the dashboard.

### 6. Bump minor version on dev

```sh
git checkout dev
./scripts/bump-version.sh minor       # e.g. released 3.0.2 → dev becomes 3.1.0
git add VERSION && git commit -m "Bump minor version for next development cycle"
git push origin dev
```

## Hotfixes

For urgent fixes to production:

1. Checkout `beta`, apply the fix, bump the patch version
2. Merge `beta` into `main`
3. Release and publish from `main`
4. Merge the fix back into `dev`

```sh
git checkout beta
# ... fix and commit ...
git checkout main && git merge beta && git push origin main
./release.sh && ./push-release.sh
git checkout dev && git merge beta && git push origin dev
```

## Building

Qt 6.4.2 + QtWebEngine, built with CMake. There is no qmake `.pro` file —
Debian's Qt 6 ships no aarch64 mkspec, so the qmake cross-build the 2.x line
used cannot work.

### Raspberry Pi CM4 / Pi 5 (arm64)

```sh
./docker/build-cm4.sh
```

- Docker image: `rbrowser-cm4-qt6-build` (Debian 12 + `crossbuild-essential-arm64`
  and Qt 6 `:arm64` dev packages)
- Cross-compiled with `docker/toolchain-arm64.cmake`
- Output: `build-cm4/robot-browser` (ELF 64-bit ARM aarch64)

Docker caches the image after the first run, so later builds only recompile
changed sources. To force a clean build, `rm -rf build-cm4` first.

### Local (amd64)

For UI work that does not need the target. Prerequisites on Debian 12:

```sh
sudo apt-get install -y cmake qt6-base-dev qt6-webengine-dev \
    qt6-websockets-dev qt6-declarative-dev qt6-virtualkeyboard-dev
```

```sh
cmake -B build-amd64 src -DCMAKE_BUILD_TYPE=Release
cmake --build build-amd64 -j$(nproc)
./build-amd64/robot-browser http://remote-url http://local-url
```

The virtual keyboard behaves differently under a desktop session than on a
kiosk terminal, so anything keyboard-related must be confirmed on hardware.

### BeagleBone Black (armhf)

Retired. QtWebEngine needs a GPU and a compositor, and cannot run on linuxfb;
`scripts/build-deb.sh armhf` exits with an explanation. The last armhf build is
on the `webkit` branch.

## Debian Packaging

```sh
./scripts/build-deb.sh arm64    # CM4 / Pi 5 — Docker cross-compile, then package
./scripts/build-deb.sh amd64    # local build, then package
```

Output: `build-deb/robot-browser_<version>-1_<arch>.deb`

### Package contents

| Path | Description |
|---|---|
| `/usr/bin/robot-browser` | Application binary |
| `/usr/lib/robot-browser/robotbrowser.sh` | Startup wrapper |
| `/usr/lib/systemd/system/robot-browser.service` | Service unit, **not enabled** — the kiosk session starts the browser, and an enabled service fights it |
| `/etc/robot-browser/browser.config` | Configuration (preserved on upgrade) |
| `/etc/udev/rules.d/99-robot-input.rules` | Touchscreen/keyboard device symlinks |
| `/usr/share/robot-browser/layouts/` | Virtual keyboard layouts |
| `/usr/share/robot-browser/qml/` | Virtual keyboard `robot` style |

### Configuration

`/etc/robot-browser/browser.config` on the terminal:

```sh
WB_REMOTE_URL=http://192.168.100.1/transaction   # the [Remote] button, and the landing page
WB_LOCAL_URL=http://127.0.0.1                    # the [Home] button — the local web UI
NETWORK_WIFI=on                                  # auto | on | off — show the WiFi icon and dialog
NETWORK_LAN=off                                  # auto | on | off — show the LAN icon and dialog
```

`auto` shows the control when the hardware is present. Deployments differ: the
T430 is wired (`NETWORK_LAN=on`, `NETWORK_WIFI=off`), the T440 is WiFi roaming
(the reverse), general terminals leave both on `auto`.

Anything on the command line overrides the file, so a provisioning layer can
pass what it holds without this package knowing that layer exists:

```sh
robot-browser [--config=PATH] [--wifi=auto|on|off] [--lan=...] [remote_url] [local_url]
```

## Deploying to Terminals

### Via apt

On a terminal with the Radical ES repository configured — this is how
production terminals are updated, and only ever serves builds from `main`:

```sh
sudo apt-get update
sudo apt-get install robot-browser
sudo systemctl restart lightdm      # the session owns the browser process
```

### Manual install

For testing a `dev` or `beta` build without publishing it:

```sh
scp build-deb/robot-browser_<version>-1_arm64.deb robot@<ip>:/tmp/rb.deb
ssh robot@<ip> "sudo apt-get install -y -o Dpkg::Options::=--force-confold \
    --reinstall /tmp/rb.deb && sudo systemctl restart lightdm"
```

`--force-confold` keeps the terminal's own `browser.config`. Restarting
`lightdm` is what restarts the browser: the kiosk session starts it, not the
systemd unit.

For first-time terminal setup, see [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md) and
[docs/KIOSK-CM4.md](docs/KIOSK-CM4.md).

## Quick Reference

| Task | Command |
|------|---------|
| Cross-build for target | `./docker/build-cm4.sh` |
| Local build | `cmake --build build-amd64 -j$(nproc)` |
| Build package | `./scripts/build-deb.sh arm64` |
| Bump version | `./scripts/bump-version.sh patch` |
| Promote to beta | `git checkout beta && git merge dev && git push origin beta` |
| Promote to main | `git checkout main && git merge beta && git push origin main` |
| Release (from main) | `./release.sh && ./push-release.sh` |
| Release dry run | `./release.sh --dry-run` |
| Publish to CDN (from main) | `./scripts/publish-deb.sh arm64` |
| Install on terminal | `sudo apt-get update && sudo apt-get install robot-browser` |
