# Enhancements Plan — Smart Matrix Clock

## Overview

This plan incorporates new features and improvements into the existing firmware. Features are prioritised by urgency and grouped so that each sub-task can be implemented and tested independently.

Phases 4 and 5 (Weather and Quotes) from the original plan remain intact and are not affected by this plan.

Reference: [`docs/implementation-plan.md`](docs/implementation-plan.md) | [`docs/project-spec.md`](docs/project-spec.md)

---

## Status summary

### Implemented

| Sub-task | Feature |
|---|---|
| Sub-Task 1 | Configurable HH:MM:SS clock mode (`HH:MM` left-aligned with blinking colon + `SS` in 3×5 small font) |
| Sub-Task 2 | Icons/symbols in messages (`[heart]`, `[star]`, `[warn]`, etc.) |
| Sub-Task 4 | Web interface language (EN/PT, persisted on device) |
| Sub-Task 5 | WiFi network scan (`GET /api/wifi/scan`) — scan button in Network tab, SSID list with signal bars and lock icon, click-to-fill |
| Sub-Task 6a | Live message preview (icon tags resolved in browser before sending) |
| Sub-Task 6b | Automatic brightness by time of day (night window, configurable start/end/level) |
| Sub-Task 6d | Message history (`GET /api/messages/history`, last 20 messages) |
| Sub-Task 6e | Slot scheduling by time of day (per-slot daily window on weather/quotes) |

### Pending

| Sub-task | Feature | Urgency | GitHub Issue |
|---|---|---|---|
| Sub-Task 3 | Web interface password (HTTP Basic Auth) | 🟢 Low | [#8](https://github.com/jansouza/ibm-bob-matrix-clock/issues/8) |
| Sub-Task 6c | OTA firmware update (`POST /api/ota`) | 🟡 Medium | [#9](https://github.com/jansouza/ibm-bob-matrix-clock/issues/9) |
| Sub-Task 6f | Soft reboot via panel (`POST /api/restart`) | 🟡 Medium | [#10](https://github.com/jansouza/ibm-bob-matrix-clock/issues/10) |
| Sub-Task 7 | 12-hour clock mode (HH:MM AM/PM) | 🟡 Medium | [#11](https://github.com/jansouza/ibm-bob-matrix-clock/issues/11) |
| Sub-Task 8 | Diagnostics endpoint (`GET /api/diag`) | 🟡 Medium | [#12](https://github.com/jansouza/ibm-bob-matrix-clock/issues/12) |
| Sub-Task 9 | Async WiFi scan (non-blocking) | 🟡 Medium | [#13](https://github.com/jansouza/ibm-bob-matrix-clock/issues/13) |
| Sub-Task 10 | MQTT push messages | 🟢 Low | [#14](https://github.com/jansouza/ibm-bob-matrix-clock/issues/14) |
| Sub-Task 11 | Display test mode (`POST /api/test`) | 🟢 Low | [#15](https://github.com/jansouza/ibm-bob-matrix-clock/issues/15) |
| Sub-Task 12 | Message queue / playlist | 🟢 Low | [#16](https://github.com/jansouza/ibm-bob-matrix-clock/issues/16) |
| Sub-Task 13 | Version control & GitHub Actions release pipeline (version part only — `FIRMWARE_VERSION`, `/api/status` field, web panel footer) | 🟡 Medium | [#17](https://github.com/jansouza/ibm-bob-matrix-clock/issues/17) |

---

## Code fixes

Code-level issues found during review — bugs, robustness gaps, and cleanups that don't require a new feature. Each item is self-contained and can be fixed independently.

→ Full details, problem descriptions, and code snippets: **[`docs/code-fixes.md`](code-fixes.md)**

| # | Severity | Summary | File |
|---|---|---|---|
| C1 | ⚪ Not an issue | ~~`applyTimezone()` UB when IANA lookup returns `nullptr` twice~~ ✅ invalid — stale comment fixed | `persistence.cpp` |
| C2 | 🟡 Bug Risk | ~~`_slotInSchedule()` uses epoch `tm_wday` before NTP syncs~~ ✅ fixed | `display.cpp` |
| C3 | 🟢 Cleanup | `slotEnabled[2/3]` written twice in `loadConfig()` — generic keys redundant | `persistence.cpp` |
| C4 | 🟡 Robustness | ~~`_fetchOneQuote()` — no guard for empty `getString()` on heap OOM~~ ✅ fixed (both fetchers) | `data_fetcher.cpp` |
| C5 | 🟡 Robustness | Factory reset BOOT button has no debounce — USB DTR can wipe settings | `.ino · setup()` |
| C6 | 🟢 Cleanup | Slot indices `2`/`3` hardcoded everywhere — add `SLOT_WEATHER`/`SLOT_QUOTES` constants | `config.h` |
| C7 | 🟢 Cleanup | ~~`_fetchWeather()` URL builder duplicated for °C/°F — use single `snprintf`~~ ✅ fixed | `data_fetcher.cpp` |

---

## Priorities

| # | Feature | Urgency |
|---|---|---|
| F1 | Seconds clock mode (HH:MM:SS) | 🔴 High |
| F2 | Icons/symbols in messages | 🟡 Medium |
| F3 | Password for the web interface (HTTP Basic Auth) | 🟢 Low |
| F4 | Web interface language (pt/en in browser) | 🟢 Low |
| F5 | WiFi network scan in the web interface | 🟢 Low |
| F6 | Additional improvement suggestions | 🟡 Medium |
| F7 | Soft reboot via panel (`POST /api/restart`) | 🟡 Medium |
| F8 | 12-hour clock mode (HH:MM AM/PM) | 🟡 Medium |
| F9 | OTA firmware update (`POST /api/ota`) | 🟡 Medium |
| F10 | Diagnostics endpoint (`GET /api/diag`) | 🟡 Medium |
| F11 | Async WiFi scan (non-blocking two-step) | 🟡 Medium |
| F12 | MQTT push messages | 🟢 Low |
| F13 | Display test mode (`POST /api/test`) | 🟢 Low |
| F14 | Message queue / playlist | 🟢 Low |

---

## Sub-Task 1 — Configurable HH:MM:SS mode (High urgency)

**Status:** `[x] done`

> **Implementation note:** follows the original design — `HH:MM` in normal font left-aligned with blinking colon every 500 ms, and `SS` overlaid in the 3×5 `_dateSmallFont` at visual columns `SS_COL_START` (21) and `SS_COL_START+4` (25) via direct `setColumn()` calls. `_writeSmallDigit()` navigates the PROGMEM font table; `_renderHHMMSS()` orchestrates the full render. `displayForceRedraw()` is called by `web_routes.cpp` when `clock_mode` changes at runtime.

### Intent

Add an alternative clock mode where the display shows `HH:MM` in the standard font aligned left and `SS` (no colon) in a 3×5 font (`date_font.h`) overlaid on the final columns via direct `setColumn()` calls to the driver. The mode is configurable from the web panel and persists in NVS. When enabled, it fully replaces `HH:MM` mode; when disabled, it returns to the default behaviour with a blinking colon.

### Design decision

The display has 32 columns (4 FC16 modules × 8 columns). The approach of mixing two fonts in the same frame works as follows:

1. **Render `HH:MM`** with `_display.setTextAlignment(PA_LEFT)` + `_display.print()` using the default font, left-aligned. `HH:MM` in normal font occupies ~20 columns — it sits in columns 0–19, leaving columns 20–31 free on the right.
2. **Write `SS`** directly into columns 21–31 using `_display.getGraphicObject()->setColumn(col, byte)`. Glyph bytes are read from the PROGMEM of `_dateSmallFont` using `pgm_read_byte()`. Each digit occupies 3 columns + 1 separator column.
3. The blinking colon continues inside `HH:MM` every 500 ms (same behaviour as the standard mode). Seconds update every 1 s (separate check via `millis()`).

**Why not use two consecutive `_display.print()` calls:** MD_Parola only has one active text buffer. The second `print()` clears the entire display before redrawing. Direct access via `getGraphicObject()->setColumn()` does not clear — it only overwrites the specific columns.

**Helper function `_writeSmallDigit(col, char digit)`:** reads the digit entry from `_dateSmallFont` (via `pgm_read_byte` from PROGMEM), writes each column to the display. The font defines width + bytes per glyph, so the table must be iterated to find the correct character offset.

**`SS` start column:** fixed at column 21 (leaves 1 column of margin after `HH:MM`). Digit0 = cols 21–23, gap = col 24, Digit1 = cols 25–27. Columns 28–31 stay blank.

### Expected Outcomes

- A "Show seconds" toggle in the Clock tab of the web panel.
- When enabled: `HH:MM` left-aligned in normal font (with blinking colon at 500 ms) and `SS` in 3×5 font in columns 21–27, updated every second.
- When disabled: display returns to centred `HH:MM` in normal font with blinking colon — current behaviour unchanged.
- The preference persists after reboot.
- No `delay()` — colon blink at 500 ms and seconds update at 1 s controlled by independent `millis()` timers.

### Todo List

1. **`config.h`** — Add `#define CLOCK_MODE_HHMM 0` and `#define CLOCK_MODE_HHMMSS 1`; add NVS key `NVS_KEY_CLOCK_MODE "clock_mode"`. Add `#define SS_COL_START 21` (seconds start column).
2. **`globals.h/cpp`** — Declare and define `uint8_t cfgClockMode` (default `CLOCK_MODE_HHMM`).
3. **`persistence.cpp`** — In `loadConfig()`, read `cfgClockMode` from NVS with fallback 0. In `saveConfig()`, persist `cfgClockMode`.
4. **`display.cpp`** — Add:
   - `static uint32_t _lastSecondUpdate = 0` and `static uint8_t _lastSec = 0xFF` for seconds update control.
   - Helper function `_writeSmallDigit(uint8_t startCol, char digit)` that iterates `_dateSmallFont` in PROGMEM to locate the digit glyph offset and writes each column via `_display.getGraphicObject()->setColumn()`.
   - Function `_renderHHMMSS(struct tm& t, bool colonVisible)`: calls `_display.setFont(nullptr)`, `setTextAlignment(PA_LEFT)`, `print("HH:MM" or "HH MM")` as in the current mode; then calls `_writeSmallDigit` for the two digits of `t.tm_sec` at columns `SS_COL_START` and `SS_COL_START + 4`.
   - In the colon blink block (line 234): at the end, if `cfgClockMode == CLOCK_MODE_HHMMSS`, call `_renderHHMMSS()` instead of the normal (centred) `print()`. Separately check if the second changed (1 s timer) to update only the `SS` digits without redrawing `HH:MM`.
   - When leaving `CLOCK_MODE_HHMMSS` (config change, date, message): call `_display.setTextAlignment(PA_CENTER)` and clear `_lastTimeStr`.
5. **`web_routes.cpp`** — In `POST /api/config`, accept field `clock_mode` (0 or 1), validate, save, apply (update `cfgClockMode`, force redraw by clearing `_lastTimeStr` via `displayForceRedraw()`).
6. **`display.h/cpp`** — Expose `void displayForceRedraw()` that clears `_lastTimeStr[0] = '\0'` to ensure the next `displayTick()` call redraws from scratch.
7. **`web_page.h`** — In the Clock tab, add a "Show seconds" toggle that reads `clock_mode` from `GET /api/config` and writes it via `POST /api/config`. Live preview should show simulated `:SS` when active.

### Relevant Context

- [`smart-matrix-clock-esp32/display.cpp:233–261`](smart-matrix-clock-esp32/display.cpp:233) — current colon blink and `HH:MM` rendering block.
- [`smart-matrix-clock-esp32/date_font.h`](smart-matrix-clock-esp32/date_font.h) — 3×5 font in PROGMEM; format: `<width>, <col0>, <col1>, [col2]` per glyph; header `'F', 1, firstChar=32, lastChar=90, 8`.
- `MD_Parola::getGraphicObject()` returns a pointer to the underlying `MD_MAX72XX`; `MD_MAX72XX::setColumn(module, col, data)` writes a byte to a specific column.
- [`smart-matrix-clock-esp32/config.h`](smart-matrix-clock-esp32/config.h) — location of constants and NVS keys.
- [`smart-matrix-clock-esp32/persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp) — NVS load/save pattern to follow.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — field validation pattern in POST /api/config.

---

## Sub-Task 2 — Icons/symbols in messages (Medium urgency)

**Status:** `[x] done`

### Intent

Allow message text to contain tags like `[heart]`, `[star]`, `[smile]` that are replaced by CP437 special character bytes (already present in the MD_MAX72XX default font) before rendering. The web interface displays a panel of symbol buttons that automatically insert tags into the message text field.

CP437 characters are mapped to MD_MAX72XX font indices (control characters 0x01–0x1F that the driver renders as special glyphs). No font or driver modifications are required.

### Expected Outcomes

- A list of ~10 common icons available (heart, star, smile, arrow, bell, etc.) with mappings to CP437 bytes.
- Text `"Warning [bell] test"` arrives at the API as a string, and before being passed to the display it is processed to replace `[bell]` with the corresponding byte.
- The Message tab in the web UI displays a row of buttons with icons (using equivalent Unicode characters as visual labels) that insert tags into the text field.
- The API documentation lists the supported icon names.

### Todo List

1. **`text_encoding.h/cpp`** — Add function `expandIconTags(const char* src, char* dst, size_t maxLen)` that scans the string looking for `[name]` patterns and replaces them with the mapped CP437 byte. Mapping defined in a static internal table (name → byte).
   - Initial icons: `heart`→0x03, `diamond`→0x04, `club`→0x05, `spade`→0x06, `bullet`→0x07, `smile`→0x01, `star`→0x0F, `arrow_right`→0x10, `arrow_left`→0x11, `bell`→0x0D, `note`→0x0E.
2. **`web_routes.cpp`** — In the `POST /api/message` handler, after `utf8ToLatin1()`, call `expandIconTags()` before copying to `messageText[]`.
3. **`web_page.h`** — In the Message tab, add a row of icon buttons below the text field. Each button displays the corresponding Unicode glyph (e.g.: ♥ ★ ☺) and on click inserts the tag `[heart]` etc. into the text field at the cursor position.
4. **`docs/api-rest.md`** — Document the list of supported icon tags and their glyphs.

### Relevant Context

- [`smart-matrix-clock-esp32/text_encoding.h/cpp`](smart-matrix-clock-esp32/text_encoding.h) — natural location for the tag expansion function; pipeline already passes through `utf8ToLatin1()`.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — `POST /api/message` handler where the expansion must be inserted.
- [`smart-matrix-clock-esp32/web_page.h`](smart-matrix-clock-esp32/web_page.h) — Message tab in the web UI.
- CP437 character set: bytes 0x01–0x1F are special glyphs (smile, suit symbols, etc.).

---

## Sub-Task 3 — Web interface password via HTTP Basic Auth (Low urgency) [#8](https://github.com/jansouza/ibm-bob-matrix-clock/issues/8)

**Status:** `[ ] pending`

### Intent

Protect access to the web interface with HTTP Basic Auth. The user configures a password (no username field, or fixed username `admin`) in the panel, and the ESP32 validates the `Authorization: Basic <base64>` header on all routes except the initial configuration route. Protection is optional (off by default) and can be disabled from the panel.

### Design lesson from the removed API-key attempt

A previous attempt at this used a custom `X-API-Key` header (`cfgApiAuthEnabled` / `cfgApiKey`, checked via `_checkApiKey()` in `web_routes.cpp`). It was implemented and later **removed** because the built-in web panel's own JavaScript never attached the header on any of its `fetch()` calls — as soon as a user enabled auth from the panel's own toggle, every subsequent POST from the panel itself (including that same toggle's save button, and "Send message") started returning `401 Unauthorized`, locking the panel out of itself. There was also no place in the UI to persist the key for reuse (it was only shown once, on demand, for copying into external tools like `curl`).

**Takeaway for this sub-task:** any new auth mechanism must cover the web panel's own requests, not just external/programmatic callers. HTTP Basic Auth sidesteps the original failure mode because the browser itself caches and resends the `Authorization` header automatically once the user enters credentials in the native prompt — no JS-side header plumbing is required. Do not reintroduce a custom header scheme without also wiring the panel's `fetch()` calls to send it.

### Expected Outcomes

- A "Protect web interface" toggle and password field in a new Security tab of the panel (the previous API tab was removed along with the API-key attempt — see the design lesson above).
- When active, any access to `/` or API routes responds with `401 Unauthorized` with header `WWW-Authenticate: Basic realm="SmartMatrix"` if the credential header is absent or incorrect.
- The browser displays the native HTTP Basic Auth login box.
- The password is stored in NVS (plain text, given the local embedded context).
- The `POST /api/config` route with fields `web_password` and `web_auth_enabled` updates the settings.

### Todo List

1. **`config.h`** — Add `#define WEB_PASS_MAX 33` and `NVS_KEY_WEB_AUTH "web_auth_en"` and `NVS_KEY_WEB_PASS "web_pass"`.
2. **`globals.h/cpp`** — Declare `bool cfgWebAuthEnabled` and `char cfgWebPassword[WEB_PASS_MAX]`.
3. **`persistence.cpp`** — Read/save `cfgWebAuthEnabled` and `cfgWebPassword` in NVS.
4. **`web_routes.cpp`** — Implement helper function `_checkBasicAuth(AsyncWebServerRequest* req)` that:
   - Decodes the `Authorization: Basic <b64>` header.
   - Compares with `cfgWebPassword` (username ignored or fixed `admin`).
   - Returns `true` if authorised or if `cfgWebAuthEnabled == false`.
   - On routes `GET /`, `GET /api/config`, `POST /api/config`, `POST /api/message`, `GET /api/status`, `POST /api/wifi`: call `_checkBasicAuth()` at the start; if false, respond 401.
5. **`web_page.h`** — In a new Security tab, add a "Protect web interface" toggle and password field; reads/writes via `POST /api/config` with fields `web_auth_enabled` and `web_password`.

### Relevant Context

- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — location where the auth check must be added.
- ESPAsyncWebServer provides `request->hasHeader()` and `request->getHeader()` to read HTTP headers.
- Base64 decode: use the built-in Arduino ESP32 `base64` library (`#include <base64.h>`).

---

## Sub-Task 4 — Web interface language in browser (Low urgency)

**Status:** `[x] done`

### Intent

Add a language selector (EN / PT, extensible to more languages later) in the web UI that translates all labels, texts and interface messages. Unlike the original draft of this sub-task, the preference is **persisted on the ESP32** (NVS) via `POST /api/config`, not just in browser `localStorage` — so the choice survives across browsers/devices accessing the panel. Default language is **English (`en`)**. The display language (clock/date weekday-month names, `cfgLanguage`) is a separate, pre-existing setting and remains fully independent from this one.

### Design decision

- A new persisted field `cfgUiLanguage` (distinct from the existing `cfgLanguage`, which only controls the on-device clock/date locale) stores the web UI language code, e.g. `"en"`, `"pt"`.
- The set of valid codes is **not hardcoded as a two-way if/else**: both firmware validation and the front-end dictionary lookup are structured as a small table/object keyed by language code, so adding a third language later means adding one entry, not branching logic.
- On page load, the UI fetches `GET /api/config`, reads `ui_language`, and calls `applyLang()` with it (falling back to `en` if the stored/received value has no matching dictionary — this keeps unknown future values, or a value from a newer firmware, from breaking the page).
- Changing the selector immediately re-renders the UI client-side (no reload) **and** persists the choice via `POST /api/config` with `{ "ui_language": "<code>" }`, so it's saved for future visits from any browser.

### Expected Outcomes

- A language selector visible on all tabs (e.g.: top-right corner of the header), defaulting to English on a factory-reset device.
- When switching, all UI texts change instantly without reloading the page, and the choice is saved to the device.
- Reopening the panel (even from a different browser/computer) loads the previously saved language from the ESP32.
- Translation coverage: all form labels, button texts, placeholders, toast feedback messages and tab titles.
- Adding a new language in the future requires: one new dictionary object in `I18N`, one new `<option>` in the selector, and one new entry in the firmware's allowed-language table — no structural rework.

### Todo List

1. **`config.h`** — Add `#define UI_LANG_CODE_MAX 4` and `NVS_KEY_UI_LANGUAGE "ui_lang"`. Add `#define UI_LANG_DEFAULT "en"`. Define the allowed UI language list once as an array (e.g. `static const char* const UI_LANGUAGES[] = {"en", "pt"};`) so validation and future additions stay in one place.
2. **`globals.h/cpp`** — Declare and define `char cfgUiLanguage[UI_LANG_CODE_MAX]` (default `UI_LANG_DEFAULT`). Keep this separate from `cfgLanguage` (display locale).
3. **`persistence.cpp`** — In `loadConfig()`, read `cfgUiLanguage` from NVS with fallback `UI_LANG_DEFAULT`. In `saveConfig()`, persist `cfgUiLanguage`. In `factoryReset()`, reset it to `UI_LANG_DEFAULT`.
4. **`web_routes.cpp`** — In `GET /api/config`, add `doc["ui_language"] = cfgUiLanguage`. In `POST /api/config`, accept field `ui_language`; validate it against the `UI_LANGUAGES` table (loop, not if/else chain) and reject unknown codes with 400; on success copy into `cfgUiLanguage` and mark `changed = true`.
5. **`web_page.h`** — Create a JavaScript `I18N` object keyed by language code (`{ en: {...}, pt: {...} }`) containing all UI strings, structured so a new key can be added without touching lookup code.
6. **`web_page.h`** — Implement function `applyLang(lang)` that falls back to `en` if `lang` isn't a known key in `I18N`, then traverses the DOM replacing `data-i18n="key"` elements with the matching dictionary text. Add `data-i18n` attributes to all text elements in the UI.
7. **`web_page.h`** — Add a language `<select>` (or button group) in the header, populated from `Object.keys(I18N)` rather than hardcoded markup where practical, so the option list and the dictionary stay in sync. On change: call `applyLang()` immediately, then `POST /api/config` with `{ui_language: <code>}`.
8. **`web_page.h`** — In `loadConfig()` (existing function that reads `GET /api/config`), read `c.ui_language` and call `applyLang(c.ui_language || 'en')` and set the selector's value to match.

### Relevant Context

- [`smart-matrix-clock-esp32/globals.h`](smart-matrix-clock-esp32/globals.h) — existing `cfgLanguage`/`LANG_DEFAULT` ("pt") controls only the display's day/month names via `locale_data`; do not reuse or repurpose this field for the web UI language.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — `POST /api/config` field validation pattern to follow (see the existing `language` field handling as a precedent, but keep `ui_language` fully separate).
- [`smart-matrix-clock-esp32/persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp) — NVS load/save/factory-reset pattern to extend.
- [`smart-matrix-clock-esp32/web_page.h`](smart-matrix-clock-esp32/web_page.h) — the entire UI is in a single string literal; most of this sub-task's front-end work lives here.

---

## Sub-Task 5 — WiFi network scan in the web interface (Low urgency)

**Status:** `[x] done`

> **Implementation note:** the scan runs synchronously (`WiFi.scanNetworks(false)`) inside `_handleGetWifiScan()` in `web_routes.cpp` — acceptable since it is user-triggered only and follows the same blocking-in-handler rationale as `_stationConnect()`. The Network tab now has a "Scan networks" button next to the SSID field; results are sorted by RSSI descending and rendered as clickable rows showing lock/open icon, SSID name and a 4-bar signal indicator with dBm value. Clicking a row fills the SSID field and focuses the password input. `WiFi.scanDelete()` is called after each scan to free driver memory. Fully localised in EN and PT via the `I18N` dictionary.

### Intent

Add a "Scan networks" button in the Network tab that triggers a WiFi scan on the ESP32 via a new `GET /api/wifi/scan` endpoint. The result lists found networks with SSID, signal strength (RSSI → bars) and a security indicator (🔒). Clicking a network automatically fills in the SSID field.

The scan is synchronous (blocks ~2–3 s) and must only occur when explicitly triggered by the button — never automatically at boot or in the loop.

### Expected Outcomes

- A "Scan networks" button in the Network tab of the web UI.
- On click, the button shows "Scanning..." state while waiting for the response.
- The network list appears below the button with: network name, signal bars (based on RSSI) and a lock icon if encrypted.
- Clicking a network in the list automatically fills the SSID form field.
- `GET /api/wifi/scan` returns JSON: `[{"ssid":"NetworkName","rssi":-65,"secure":true}, ...]`.

### Todo List

1. ~~**`wifi_manager.h/cpp`** — Add function `wifiScan(JsonArray& results)`~~ — scan implemented inline in the route handler (`_handleGetWifiScan`) in `web_routes.cpp` to keep `wifi_manager` scope-limited to connection management.
2. **`web_routes.cpp`** ✅ — `GET /api/wifi/scan` registered; handler calls `WiFi.scanNetworks()`, builds JSON array `[{ssid, rssi, secure}]`, calls `WiFi.scanDelete()`, responds.
3. **`web_page.h`** ✅ — Network tab updated: "Scan networks" button inline with SSID field; `runWifiScan()` renders sorted results; click-to-fill; I18N keys in EN + PT.

### Relevant Context

- [`smart-matrix-clock-esp32/wifi_manager.h/cpp`](smart-matrix-clock-esp32/wifi_manager.h) — natural location for `wifiScan()`.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — where to register the new route.
- `WiFi.scanNetworks()` returns `int` (number of networks); `WiFi.SSID(i)`, `WiFi.RSSI(i)`, `WiFi.encryptionType(i)` retrieve the data.
- The scan blocks ~2–3 s — acceptable since it only occurs on explicit user demand, outside the normal loop.

---

## Sub-Task 6 — Additional suggested improvements (Medium urgency) · [6c #9](https://github.com/jansouza/ibm-bob-matrix-clock/issues/9) · [6f #10](https://github.com/jansouza/ibm-bob-matrix-clock/issues/10)

**Status:** `[ ] pending`

### Intent

Incorporate the following improvements into the product roadmap, all compatible with the existing architecture:

| # | Improvement | Rationale |
|---|---|---|
| 6a `[x] done` | **Live message preview** | The current preview in the Clock tab simulates the display. Add a preview in the Message tab that shows the formatted text (with resolved icons) before sending |
| 6b `[x] done` | **Automatic brightness by time of day** | Automatically reduce brightness at night (e.g.: 23h–7h) to avoid disturbance. Configurable: start/end time and night brightness level |
| 6c | **OTA (Over-The-Air) firmware update** | `POST /api/ota` endpoint accepts a `.bin` file, writes via ESP32 `Update.h`. Reduces the need for a USB cable for updates |
| 6d `[x] done` | **Message history** | Maintain a ring buffer of the last N received messages (timestamp + text). Exposed via `GET /api/messages/history`. Useful for debugging and auditing |
| 6e `[x] done` | **Slot scheduling by time of day** | Configure time windows in which a slot is displayed (e.g.: quotes only 9h–18h on weekdays). Requires only one extra field per slot and a check in `slotRotationTick()` |
| 6f | **Soft reboot via panel** | "Restart device" button in the Network/API tab that calls `POST /api/restart`; triggers `scheduleRestart(1500)`. Useful after complex configuration changes |

### Todo List

Each item 6a–6f should be treated as an independent sub-task when approved for implementation:

- **6a** — Modify only `web_page.h`: add a preview div in the Message tab with JavaScript tag-resolution logic.
- **6b** — `config.h` (constants), `globals.h/cpp` (cfgNightBrightnessEnabled, cfgNightStart, cfgNightEnd, cfgNightBrightness), `persistence.cpp`, `display.cpp` (check in `displayTick()`), `web_page.h` (controls in Clock tab), `web_routes.cpp`.
- **6c** — `web_routes.cpp` (OTA endpoint with `Update.h`), `web_page.h` (upload button in API tab or new tab).
- **6d** — `globals.h/cpp` (message ring buffer), `web_routes.cpp` (`GET /api/messages/history`), `web_page.h` (display in Message tab).
- **6e** — `config.h` (scheduling fields), `globals.h/cpp`, `persistence.cpp`, `display.cpp` (`slotRotationTick()`), `web_page.h`, `web_routes.cpp`.
- **6f** — `web_routes.cpp` (`POST /api/restart` calling `scheduleRestart(1500)`), `web_page.h` (button in Network tab).

### Relevant Context

- Items 6b and 6e depend on Sub-Task 1 (HH:MM:SS mode) being functional to avoid conflicts with `displayTick()` logic.
- Item 6c (`Update.h`) is built into the ESP32 Arduino core — no external libraries required.
- Item 6f (`scheduleRestart`) already exists in [`wifi_manager.cpp`](smart-matrix-clock-esp32/wifi_manager.cpp) — simply expose it via an HTTP route.

---

## Recommended Implementation Order

```
[Done] Sub-Task 1 (HH:MM:SS)
[Done] Sub-Task 2 (Icons)
[Done] Sub-Task 4 (UI i18n)
[Done] Sub-Task 5 (WiFi Scan)
[Done] Sub-Task 6a (Live preview)
[Done] Sub-Task 6b (Auto brightness)
[Done] Sub-Task 6d (Message history)
[Done] Sub-Task 6e (Slot scheduling)

Code fixes C1–C7                ← targeted fixes; no new features
Sub-Task 6f / F7 (Restart)     ← trivial; 3 lines firmware + 1 button in UI
Sub-Task 7 / F8 (12h clock)    ← plan already in docs/clock-12h-ampm-plan.md
Sub-Task 8 / F10 (Diagnostics) ← single handler; no UI required
Sub-Task 6c / F9 (OTA)         ← moderate complexity; high user value
Sub-Task 9 / F11 (Async scan)  ← improves current WiFi scan
Sub-Task 3 (HTTP Basic Auth)   ← security; must not break web panel self-requests
Sub-Task 10 / F12 (MQTT)       ← extra library; most useful for home automation users
Sub-Task 11 / F13 (Test mode)  ← hardware verification utility
Sub-Task 12 / F14 (Msg queue)  ← UX improvement; touches message slot logic
```

---

## Compatibility with Phases 4 and 5

This plan does not conflict with Phases 4 (Weather) and 5 (Quotes) from [`docs/implementation-plan.md`](docs/implementation-plan.md). The sub-tasks described here modify:
- **Display** (Sub-Task 1): only the colon-blink block — Phase 4 additions are in `slotRotationTick()` which does not exist yet.
- **text_encoding** (Sub-Task 2): function addition, without modifying existing ones.
- **web_routes** (Sub-Tasks 3, 5): route and middleware additions — does not alter existing handlers.
- **web_page** (Sub-Tasks 2, 3, 4, 5): UI modifications do not affect firmware.

Phases 4 and 5 can be implemented in parallel or after this plan, without conflict.

---

## Sub-Task 7 — 12-hour clock mode (HH:MM AM/PM) [#11](https://github.com/jansouza/ibm-bob-matrix-clock/issues/11)

**Status:** `[ ] pending`

### Intent

Add a 12-hour display mode as an alternative to the existing 24-hour modes. When active, the clock shows `H:MM` or `H:MM:SS` with a compact `A`/`P` suffix for AM/PM, fitting within the 32-column display. The mode is configurable from the web panel and persists in NVS. See [`docs/clock-12h-ampm-plan.md`](clock-12h-ampm-plan.md) for the pre-written detailed plan.

### Expected Outcomes

- A "12-hour mode" toggle in the Clock tab.
- Display shows `3:47P` or `3:47:22P` depending on whether seconds mode is also active.
- Colon blink behaviour is preserved.
- The preference persists after reboot.

### Todo List

1. **`config.h`** — Add `#define CLOCK_MODE_12H 2`; add `NVS_KEY_CLOCK_12H`.
2. **`globals.h/cpp`** — Declare `bool cfgClock12h`.
3. **`persistence.cpp`** — Load/save `cfgClock12h`.
4. **`display.cpp`** — In the `CLOCK_MODE_HHMM` branch, check `cfgClock12h` and format with `% 12` + AM/PM suffix.
5. **`web_routes.cpp`** — Accept `clock_12h` field in `POST /api/config`.
6. **`web_page.h`** — Add toggle in Clock tab; update live preview logic.

### Relevant Context

- [`docs/clock-12h-ampm-plan.md`](clock-12h-ampm-plan.md) — detailed pre-written plan.
- [`smart-matrix-clock-esp32/display.cpp`](../smart-matrix-clock-esp32/display.cpp) — `displayTick()` clock render block.

---

## Sub-Task 8 — Diagnostics endpoint (`GET /api/diag`) [#12](https://github.com/jansouza/ibm-bob-matrix-clock/issues/12)

**Status:** `[ ] pending`

### Intent

Expose a lightweight diagnostics endpoint that returns the device's runtime health metrics. Zero runtime overhead when not called — all values are queried on-demand from existing ESP32 APIs.

### Expected Outcomes

- `GET /api/diag` returns a JSON object with:
  - `uptime_ms` — milliseconds since boot (`millis()`)
  - `free_heap` — free heap bytes (`ESP.getFreeHeap()`)
  - `min_free_heap` — historical minimum (`ESP.getMinFreeHeap()`)
  - `wifi_rssi` — dBm (`WiFi.RSSI()`), or `null` if not connected
  - `ntp_synced` — bool
  - `last_weather_fetch_ms` — millis() of last successful weather fetch (or `0`)
  - `last_quotes_fetch_ms` — millis() of last successful quotes fetch (or `0`)
  - `build_date` — firmware build date (`__DATE__ " " __TIME__`)
- No web panel changes required (accessible via `curl` or future panel tab).

### Todo List

1. **`web_routes.cpp`** — Add `_handleGetDiag()` handler; register `GET /api/diag`.
2. **`data_fetcher.h/cpp`** — Expose `fetcherLastWeatherMs()` and `fetcherLastQuotesMs()` accessors.
3. **`docs/api-rest.md`** — Document the new endpoint.

### Relevant Context

- `ESP.getFreeHeap()` and `ESP.getMinFreeHeap()` are standard Arduino ESP32 APIs.
- `millis()` is always available; no extra state needed.

---

## Sub-Task 9 — Async WiFi scan (non-blocking) [#13](https://github.com/jansouza/ibm-bob-matrix-clock/issues/13)

**Status:** `[ ] pending`

### Intent

Replace the current synchronous `WiFi.scanNetworks(false)` in `GET /api/wifi/scan` with a two-step async approach: a first call starts the scan and returns immediately, a second call returns the results once ready. Eliminates the 2-second freeze of the ESPAsyncWebServer task during scan.

### Expected Outcomes

- `GET /api/wifi/scan` — starts scan if not already running; returns `{"status":"scanning"}` immediately. If results are already available from a previous scan, returns them instead.
- `GET /api/wifi/scan` polled again — when `WiFi.scanComplete()` is ≥ 0, returns the full results array and clears the scan.
- The web panel polls every 1 s until results arrive, then renders the list.
- The display loop is not blocked during the scan.

### Todo List

1. **`web_routes.cpp`** — Replace blocking `WiFi.scanNetworks(false)` with `WiFi.scanNetworks(true)` (async flag). Use `WiFi.scanComplete()` to check if results are ready; if not, return `{"status":"scanning"}`; if ready, build and return the results array.
2. **`web_page.h`** — Update `runWifiScan()` to re-poll `GET /api/wifi/scan` every 1 s until `status` is not `"scanning"`.

---

## Sub-Task 10 — MQTT push messages [#14](https://github.com/jansouza/ibm-bob-matrix-clock/issues/14)

**Status:** `[ ] pending`

### Intent

Allow external systems (Home Assistant, Node-RED, Zapier, n8n) to push messages to the display without polling the REST API. The ESP32 subscribes to a configurable MQTT broker + topic; arriving payloads are treated exactly like `POST /api/message` messages, including the same icon tag expansion.

### Expected Outcomes

- New MQTT settings in the web panel: broker hostname, port, topic, username/password (optional).
- When enabled, any MQTT message arriving on the subscribed topic is immediately queued as a display message.
- Supports both plain-text payloads and JSON payloads identical to `POST /api/message` body.
- If the broker is unreachable, the rest of the firmware continues normally (MQTT is non-blocking, reconnects in background).

### Todo List

1. **`config.h`** — Add `MQTT_HOST_MAX`, `MQTT_TOPIC_MAX`, `MQTT_USER_MAX`, `MQTT_PASS_MAX` and matching NVS keys.
2. **`globals.h/cpp`** — Declare `cfgMqttEnabled`, `cfgMqttHost`, `cfgMqttPort`, `cfgMqttTopic`, `cfgMqttUser`, `cfgMqttPass`.
3. **`persistence.cpp`** — Load/save MQTT settings.
4. **New `mqtt_client.h/cpp`** — Thin wrapper around `PubSubClient`; `mqttBegin()` called from `setup()`, `mqttTick()` called from `loop()`. On message: decode payload, fill `messageText`, set `messagePending = true`.
5. **`web_routes.cpp`** — Accept MQTT fields in `POST /api/config`.
6. **`web_page.h`** — New "Notifications" card in a suitable tab.

### Relevant Context

- **Required library:** `PubSubClient` by Nick O'Leary — `arduino-cli lib install "PubSubClient"`.
- Payload decode should reuse `utf8ToLatin1()` + `expandIconTags()` from `text_encoding.cpp` — same pipeline as the REST handler.

---

## Sub-Task 11 — Display test mode (`POST /api/test`) [#15](https://github.com/jansouza/ibm-bob-matrix-clock/issues/15)

**Status:** `[ ] pending`

### Intent

Add a hardware verification endpoint that lights all LEDs at maximum brightness for a configurable duration. Useful after assembly to verify there are no dead pixels or faulty modules, and for demonstrating the hardware.

### Expected Outcomes

- `POST /api/test` with body `{"mode":"all_on","duration_ms":3000}` lights all pixels at full brightness for 3 seconds, then returns the display to normal.
- `mode` values: `"all_on"` (all LEDs on), `"all_off"` (clear), `"checkerboard"` (alternating pixels).
- Implemented as a flag consumed by `displayTick()`, not blocking the handler.

### Todo List

1. **`globals.h/cpp`** — Add `testModePending` bool and `testModePattern` / `testModeDurationMs`.
2. **`display.cpp`** — Handle `testModePending` at the top of `displayTick()`: apply the pattern via `getGraphicObject()->setColumn()`, set a timer, restore after duration.
3. **`web_routes.cpp`** — Add `_handlePostTest()` handler; register `POST /api/test`.
4. **`web_page.h`** — Add "Test display" buttons in the Display tab.

---

## Sub-Task 12 — Message queue / playlist [#16](https://github.com/jansouza/ibm-bob-matrix-clock/issues/16)

**Status:** `[ ] pending`

### Intent

Currently only the most recent message is stored — sending a second one overwrites the first. A small FIFO queue (capacity configurable, default 5) allows pre-loading several messages that display in sequence before returning to the clock. This is particularly useful for automated notification systems that may send several messages in quick succession.

### Expected Outcomes

- Up to `MSG_QUEUE_SIZE` messages can be queued via successive `POST /api/message` calls.
- Messages play in order; each message uses its own mode, duration, brightness, and scroll-speed overrides.
- `GET /api/messages/queue` returns the current queue length and pending messages.
- `POST /api/messages/clear` empties the queue.

### Todo List

1. **`config.h`** — Add `#define MSG_QUEUE_SIZE 5`.
2. **`globals.h/cpp`** — Replace the single `messageText[]` / `messagePending` with a ring buffer of `MessageQueueEntry` structs (text, mode, duration, brightness, scrollSpeed).
3. **`display.cpp`** — In `displayTick()`, dequeue the next entry when the current message finishes instead of clearing `messagePending`.
4. **`web_routes.cpp`** — Update `POST /api/message` to enqueue; add `GET /api/messages/queue` and `POST /api/messages/clear`.
5. **`web_page.h`** — Show queue length in the Message tab.


## Sub-Task 13 — Version control & GitHub Actions release pipeline [#17](https://github.com/jansouza/ibm-bob-matrix-clock/issues/17)

**Status:** `[~] partial` — version string (items 1–4) implemented; GitHub Actions workflows (items 5–7) pending

### Intent

The firmware has no formal version identifier today — there is no `VERSION` constant in the source, no git tag convention, and no automated build/release artefact. This sub-task introduces:

1. A **semantic version string** embedded in the firmware (`FIRMWARE_VERSION` constant).
2. A **GitHub Actions CI workflow** that compiles the firmware on every push/PR (smoke test — no device required).
3. A **GitHub Actions release workflow** triggered by a `v*` tag that compiles the firmware, produces a `.bin` artefact, and publishes a GitHub Release with the binary attached.

### Design decisions

- Version follows **SemVer** (`MAJOR.MINOR.PATCH`), declared once in `config.h` as `#define FIRMWARE_VERSION "1.0.0"`.
- The version is exposed via `GET /api/status` (new field `firmware_version`) so the web panel can display it and OTA (Sub-Task 6c) can compare installed vs available version.
- The CI workflow uses the official `arduino/compile-sketches` GitHub Action with the `esp32:esp32:esp32` FQBN — same board as the local `arduino-cli` command.
- The release workflow runs **only** when a tag matching `v[0-9]*` is pushed; it extracts the version from the tag and asserts it matches `FIRMWARE_VERSION` in `config.h` to prevent accidental mismatches.
- Compiled `.bin` is uploaded as a release asset under the name `smart-matrix-clock-esp32-<version>.bin`.

### Expected Outcomes

- Every push to `main` or any PR triggers the CI compile check; a failing build blocks merge.
- Tagging `v1.2.3` → GitHub Release created automatically with the binary attached and release notes pre-populated from the tag annotation.
- `GET /api/status` JSON includes `"firmware_version": "1.2.3"`.
- Web panel footer (or About section) displays the firmware version string.

### Todo List

1. [x] **`config.h`** — Add `#define FIRMWARE_VERSION "1.0.0"` near the top.
2. [x] **`globals.h/cpp`** — No change needed; version is a compile-time constant.
3. [x] **`web_routes.cpp`** — In `_handleGetStatus()`, add `"firmware_version"` field from `FIRMWARE_VERSION`.
4. [x] **`web_page.h/cpp`** — Add version string to the panel footer (small muted text).
5. [ ] **`.github/workflows/ci.yml`** — Workflow: triggers on `push` and `pull_request` to `main`; installs `esp32:esp32:esp32` core + required libraries; runs `arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32`; reports pass/fail.
6. [ ] **`.github/workflows/release.yml`** — Workflow: triggers on `push` of tags matching `v[0-9]*`; compiles with `--output-dir build/`; extracts `FIRMWARE_VERSION` from `config.h` and asserts it equals the tag (strip leading `v`); creates a GitHub Release via `softprops/action-gh-release` with the `.bin` file attached.
7. [ ] **`README.md`** — Add CI badge and document the release tagging procedure.

### Relevant Context

- Build command (no device): `arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32`
- Required libraries already installed locally: `MD_MAX72XX`, `MD_Parola`, `ESP Async WebServer`, `AsyncTCP`, `ArduinoJson`.
- The `arduino/compile-sketches` action handles library and core installation declaratively via its `libraries` and `fqbn` inputs — no custom shell bootstrap needed.
- Sub-Task 6c (OTA) will consume `FIRMWARE_VERSION` to compare running firmware against an available update — implement this sub-task first so the field is already in place when OTA is built.
- Tag format: `v1.0.0`, `v1.1.0-rc1` — the release workflow should strip the leading `v` before comparing against `FIRMWARE_VERSION`.
