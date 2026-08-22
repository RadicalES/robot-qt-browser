# Writing a webapp for a Robot terminal

If you are building a web page that will run on a RadicalES Robot terminal, you
can run the terminal's own browser on your PC and develop against it. The page
you see is the page the operator sees: same engine, same viewport, same chrome
taking up the same space.

## Install

Add the RadicalES package repository (see the [README](../README.md#installing))
and install:

```sh
sudo apt-get install robot-browser
```

## Run it as a terminal

```sh
robot-browser --profile=t440 https://transactions.example.com http://localhost:3000
```

That is the whole interface: a profile, and two URLs.

| Profile | Reproduces | Viewport |
|---------|-----------|----------|
| `t430`  | Robot-T430, 7" panel, wired | 800×480 landscape |
| `t431`  | Robot-T431, 7" panel, wired | 1024×600 landscape |
| `t432`  | Robot-T432, 10" panel, wired | 1280×800 landscape |
| `t440`  | Robot-T440, 7" panel, WiFi  | 800×1280 portrait |
| `desktop` | A plain PC application | Resizable window |
| `kiosk` | The terminal itself — fullscreen on its panel | The panel |

> The sizes are the panel each terminal ships with. The T430 family is one
> terminal with more than one panel — a page laid out for a T430's 800×480 has
> 224 more pixels to fill on a T431 — so check which one your customer has.

### When the device is bigger than your screen

A T440 is 800×1280 portrait, which does not fit above a taskbar on a 1080-tall
monitor. Rather than crop the viewport — which would quietly change the thing
you are trying to look at — the browser draws the whole device smaller:

```
robot-browser: profile 't440' is 800x1280, drawn at 78% to fit this screen
(1920x1080). The page still lays out as 800x1280.
```

The window, the toolbar and the page are all multiplied by that fraction, and
the page is given it as a zoom factor. Your CSS still sees `800x1280`; only the
physical size on your monitor is smaller. A media query written for the device
matches here, and a layout that overflows the device overflows here.

Override it if you want to:

```sh
robot-browser --profile=t440 --scale=1     # 1:1, even though it will not fit
robot-browser --profile=t440 --scale=0.5   # half size
robot-browser --profile=t440 --scale=fit   # the default
```

`--scale=1` is worth using on a tall monitor, or with the window moved, when
you want real device pixels.

## Changing the URLs without restarting

The toolbar's **i** button opens System Info, which has a **Settings…** button.
It edits the same two URLs, applies them straight away, and remembers them for
next time in `~/.config/robot-browser/browser.config`. Delete that file to go
back to the defaults.

The terminals do not have this button. They are told where they point by the
file their deployment controls, and an operator who can retype a kiosk's
address is not looking at a kiosk any more.

## The two URLs

A terminal shows two fixed pages and has no address bar:

```
robot-browser [options] <remote_url> <local_url>
```

- **`remote_url`** — the transaction page. This is where operators spend their
  time, and it is what the terminal opens on. The `[Remote]` toolbar button
  returns to it.
- **`local_url`** — the terminal's own web UI, served from the device itself.
  The `[Home]` button goes here.

Point whichever one you are working on at your development server. There is no
third page and no way to type an address: if your page needs to reach somewhere
else, it has to link there.

## What a device profile gives you

**The real engine.** QtWebEngine (Chromium 102 on the current terminals), not
your desktop browser. If a feature is missing there, it is missing on the
device — this is the point of developing against it rather than against Chrome.

**The real viewport.** The bottom toolbar is 76px and it is always there. Your
page gets what is left, and it gets it at the panel's exact pixel size.

**The device's controls.** A `t430` shows a LAN button and no WiFi; a `t440`
shows WiFi and no LAN, because that is what those terminals have. The SCADA
indicator is present and grey, because your PC is not a provisioned terminal.

## What it does not give you

- **Touch.** The terminals are touch-only, and a mouse forgives things a finger
  does not. Make targets finger-sized; the toolbar's own buttons are 48px icons
  in 54px minimum-width buttons for that reason.
- **An on-screen keyboard that behaves like the device's.** It is present in a
  device profile, but the terminal's keyboard is bound up with its session in
  ways a desktop cannot reproduce exactly.
- **Anything device-side.** No `/run/robot`, no NetworkManager profiles worth
  showing, no card reader or scanner websocket on port 8100. If your page talks
  to those, mock them.
- **Reboot and Reset Defaults.** Deliberately absent — they would act on your
  workstation, not on a terminal.

## Debugging

The browser forwards JavaScript console output to a WebSocket server on port
7070, which is also how the terminals are debugged in the field.

## Reporting problems

If the browser behaves differently from the terminal, that is a bug worth
knowing about — the profile exists so that it does not. Include the profile
name, the two URLs (or a reduced page), and what the terminal does differently:
<https://github.com/RadicalES/robot-qt-browser/issues>
