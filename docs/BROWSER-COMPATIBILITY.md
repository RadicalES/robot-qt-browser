# Browser Compatibility — QtWebKit 5.212

QtWebKit 5.212 is based on WebKitGTK 2.12.5, roughly equivalent to **Safari 10 / JSC ES2016**. This document lists supported and unsupported features, polyfills injected by robot-browser, and framework compatibility.

Run `docs/feature-test.html` in robot-browser to verify results on a target device:

```bash
./robot-browser file:///path/to/docs/feature-test.html
```

## JavaScript — Native Support

| Feature | Status |
|---|---|
| Template literals | PASS |
| Arrow functions | PASS |
| let / const | PASS |
| Destructuring | PASS |
| Spread operator | PASS |
| for...of | PASS |
| Symbol | PASS |
| Promise / Promise.all / Promise.race | PASS |
| Map / Set / WeakMap / WeakSet | PASS |
| Object.assign / Object.keys | PASS |
| Array.from / Array.isArray | PASS |
| Array.find / findIndex / includes | PASS |
| String.includes / startsWith / endsWith | PASS |
| Number.isFinite | PASS |
| Class syntax | PASS |
| Default params / Rest params | PASS |
| Generator functions | PASS |
| Reflect | PASS |
| MutationObserver | PASS |
| CustomEvent | PASS |
| classList | PASS |
| XMLHttpRequest / FormData | PASS |
| DOMParser / History API | PASS |
| requestAnimationFrame | PASS |
| Element.closest / Element.matches | PASS |
| dispatchEvent | PASS |

| Feature | Status |
|---|---|
| Proxy | **FAIL** (engine limitation) |
| async / await | **FAIL** (engine limitation) |
| Optional chaining `?.` | **FAIL** (engine limitation) |
| Nullish coalescing `??` | **FAIL** (engine limitation) |

## JavaScript — Polyfilled by robot-browser

These are injected via `javaScriptWindowObjectCleared` (before page scripts run):

| Feature | Polyfill file |
|---|---|
| `NodeList.prototype.forEach` | `js/polyfills.js` |
| `Object.values` / `Object.entries` | `js/polyfills.js` |
| `URLSearchParams` | `js/polyfills.js` |
| `Array.prototype.flat` | `js/polyfills.js` |
| `String.prototype.trimStart` / `trimEnd` | `js/polyfills.js` |
| `fetch()` API | `js/fetch.js` |

## CSS — Native Support

| Feature | Status |
|---|---|
| Flexbox | PASS |
| calc() | PASS |
| transform | PASS |
| transition | PASS |
| animation | PASS |
| object-fit | PASS |

| Feature | Status |
|---|---|
| CSS Grid | **FAIL** (not polyfillable) |
| CSS Variables (Custom Properties) | **FAIL** (polyfilled, see below) |
| position: sticky | **FAIL** (polyfilled, see below) |
| gap (grid/flex) | **FAIL** (not polyfillable) |

## CSS — Polyfilled by robot-browser

These are injected on `loadFinished` (after DOM is ready):

| Polyfill | Version | Size | What it does |
|---|---|---|---|
| [css-vars-ponyfill](https://github.com/jhildenbiddle/css-vars-ponyfill) | 2.4.9 | 23 KB | Resolves `var(--name)` in `<style>` and `<link>` elements. Watches for dynamically added styles via MutationObserver. |
| [stickyfill](https://github.com/wilddeer/stickyfill) | 2.1.0 | 6.5 KB | Polyfills `position: sticky` for elements with inline `style="position: sticky"`. Top-positioned only. |
| [smoothscroll-polyfill](https://github.com/iamdustan/smoothscroll) | 0.4.4 | 4 KB | Adds `scroll-behavior: smooth` support to `scrollTo()`, `scrollBy()`, and `scrollIntoView()`. |

### CSS Variables (ponyfill) notes

- Processes `:root` and `:host` CSS custom property declarations
- `watch: true` mode re-processes when `<style>` or `<link>` elements are added/modified
- Does **not** detect changes made via CSSOM APIs (`element.style.setProperty`, `CSSStyleSheet.insertRule`)
- If a page modifies CSS variables via JavaScript CSSOM, call `cssVars()` manually from the debug console

### Stickyfill notes

- Only activates for elements with inline `style` containing `position: sticky`
- Does not auto-parse CSS stylesheets for `position: sticky` declarations
- Only supports `top` positioning (not `bottom`, `left`, `right`)
- Does not work inside `overflow: auto/hidden/scroll` containers

## Framework Compatibility

| Framework | Compatible? | Notes |
|---|---|---|
| **htmx** (1.x / 2.x) | **Yes** | All requirements met with polyfills (fetch, URLSearchParams, NodeList.forEach) |
| **Alpine.js v2** | **Yes** | Uses getters/setters, no Proxy needed |
| **Alpine.js v3** | **No** | Requires `Proxy` (engine limitation, cannot be polyfilled) |
| **jQuery** | **Yes** | Full support |
| **Tailwind CSS** | **Partial** | Works if CSS Variables are used only in `:root`. No CSS Grid utilities. |
| **Bootstrap 4** | **Yes** | Flexbox-based, no Grid or CSS Variables dependency |
| **Bootstrap 5** | **Partial** | Uses CSS Variables extensively (ponyfill helps) but some components expect CSS Grid |

## Cannot Be Polyfilled

These are rendering engine or language-level limitations:

- **CSS Grid** — layout engine feature
- **CSS gap** — tied to Grid/Flexbox engine support
- **Proxy** — JavaScript engine feature, blocks Alpine.js v3
- **async/await** — JavaScript engine feature (use Promises + `.then()` instead)
- **Optional chaining `?.`** — syntax-level feature (use `&&` chains instead)
- **Nullish coalescing `??`** — syntax-level feature (use ternary or `||` instead)
