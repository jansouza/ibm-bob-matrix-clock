# Enhancements Plan — Smart Matrix Clock

## Overview

This plan incorporates five new features into the existing firmware (Phase 3 complete) and proposes additional improvements. Features are prioritised by urgency and grouped so that each sub-task can be implemented and tested independently.

Phases 4 and 5 (Weather and Quotes) from the original plan remain intact and are not affected by this plan.

Reference: [`docs/implementation-plan.md`](docs/implementation-plan.md) | [`docs/project-spec.md`](docs/project-spec.md)

---

## Priorities

| # | Feature | Urgency |
|---|---|---|
| F1 | Seconds clock mode (HH:MM:SS) | 🔴 High |
| F2 | Icons/symbols in alert messages | 🟡 Medium |
| F3 | Password for the web interface (HTTP Basic Auth) | 🟢 Low |
| F4 | Web interface language (pt/en in browser) | 🟢 Low |
| F5 | WiFi network scan in the web interface | 🟢 Low |
| F6 | Additional improvement suggestions | 🟡 Medium |

---

## Sub-Task 1 — Configurable HH:MM:SS mode (High urgency)

**Status:** `[ ] pending`

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
   - When leaving `CLOCK_MODE_HHMMSS` (config change, date, alert): call `_display.setTextAlignment(PA_CENTER)` and clear `_lastTimeStr`.
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

## Sub-Task 2 — Icons/symbols in alert messages (Medium urgency)

**Status:** `[ ] pending`

### Intent

Allow alert text to contain tags like `[heart]`, `[star]`, `[smile]` that are replaced by CP437 special character bytes (already present in the MD_MAX72XX default font) before rendering. The web interface displays a panel of symbol buttons that automatically insert tags into the alert text field.

CP437 characters are mapped to MD_MAX72XX font indices (control characters 0x01–0x1F that the driver renders as special glyphs). No font or driver modifications are required.

### Expected Outcomes

- A list of ~10 common icons available (heart, star, smile, arrow, bell, etc.) with mappings to CP437 bytes.
- Text `"Warning [bell] test"` arrives at the API as a string, and before being passed to the display it is processed to replace `[bell]` with the corresponding byte.
- The Alert tab in the web UI displays a row of buttons with icons (using equivalent Unicode characters as visual labels) that insert tags into the text field.
- The API documentation lists the supported icon names.

### Todo List

1. **`text_encoding.h/cpp`** — Add function `expandIconTags(const char* src, char* dst, size_t maxLen)` that scans the string looking for `[name]` patterns and replaces them with the mapped CP437 byte. Mapping defined in a static internal table (name → byte).
   - Initial icons: `heart`→0x03, `diamond`→0x04, `club`→0x05, `spade`→0x06, `bullet`→0x07, `smile`→0x01, `star`→0x0F, `arrow_right`→0x10, `arrow_left`→0x11, `bell`→0x0D, `note`→0x0E.
2. **`web_routes.cpp`** — In the `POST /api/alert` handler, after `utf8ToLatin1()`, call `expandIconTags()` before copying to `alertMessage[]`.
3. **`web_page.h`** — In the Alert tab, add a row of icon buttons below the text field. Each button displays the corresponding Unicode glyph (e.g.: ♥ ★ ☺) and on click inserts the tag `[heart]` etc. into the text field at the cursor position.
4. **`docs/api-rest.md`** — Document the list of supported icon tags and their glyphs.

### Relevant Context

- [`smart-matrix-clock-esp32/text_encoding.h/cpp`](smart-matrix-clock-esp32/text_encoding.h) — natural location for the tag expansion function; pipeline already passes through `utf8ToLatin1()`.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — `POST /api/alert` handler where the expansion must be inserted.
- [`smart-matrix-clock-esp32/web_page.h`](smart-matrix-clock-esp32/web_page.h) — Alert tab in the web UI.
- CP437 character set: bytes 0x01–0x1F are special glyphs (smile, suit symbols, etc.).

---

## Sub-Task 3 — Web interface password via HTTP Basic Auth (Low urgency)

**Status:** `[ ] pending`

### Intent

Protect access to the web interface with HTTP Basic Auth. The user configures a password (no username field, or fixed username `admin`) in the panel, and the ESP32 validates the `Authorization: Basic <base64>` header on all routes except the initial configuration route. Protection is optional (off by default) and can be disabled from the panel.

### Expected Outcomes

- A "Protect web interface" toggle and password field in the API tab (or new Security tab) of the panel.
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
   - On routes `GET /`, `GET /api/config`, `POST /api/config`, `POST /api/alert`, `GET /api/status`, `POST /api/wifi`: call `_checkBasicAuth()` at the start; if false, respond 401.
5. **`web_page.h`** — In the API tab, add a "Protect web interface" toggle and password field; reads/writes via `POST /api/config` with fields `web_auth_enabled` and `web_password`.

### Relevant Context

- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — location where the auth check must be added.
- The existing API key authentication (`cfgApiAuthEnabled`, `cfgApiKey`) follows a similar pattern — reuse the same header check pattern.
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

**Status:** `[ ] pending`

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

1. **`wifi_manager.h/cpp`** — Add function `wifiScan(JsonArray& results)` that calls `WiFi.scanNetworks()` (blocking, ~2 s), iterates the results and populates the `JsonArray` with `{ssid, rssi, secure}` objects. Sort by descending RSSI.
2. **`web_routes.cpp`** — Register `GET /api/wifi/scan`: calls `wifiScan()`, serialises JSON and responds. Protect with `_checkBasicAuth()` when that feature is implemented (Sub-Task 3); for now, open route.
3. **`web_page.h`** — In the Network tab: add "Scan networks" button; on click, call `fetch('/api/wifi/scan')`, render results list; on item click, fill the SSID field.

### Relevant Context

- [`smart-matrix-clock-esp32/wifi_manager.h/cpp`](smart-matrix-clock-esp32/wifi_manager.h) — natural location for `wifiScan()`.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) — where to register the new route.
- `WiFi.scanNetworks()` returns `int` (number of networks); `WiFi.SSID(i)`, `WiFi.RSSI(i)`, `WiFi.encryptionType(i)` retrieve the data.
- The scan blocks ~2–3 s — acceptable since it only occurs on explicit user demand, outside the normal loop.

---

## Sub-Task 6 — Additional suggested improvements (Medium urgency)

**Status:** `[ ] pending`

### Intent

Incorporate the following improvements into the product roadmap, all compatible with the existing architecture:

| # | Improvement | Rationale |
|---|---|---|
| 6a | **Live alert preview** | The current preview in the Clock tab simulates the display. Add a preview in the Alert tab that shows the formatted text (with resolved icons) before sending |
| 6b | **Automatic brightness by time of day** | Automatically reduce brightness at night (e.g.: 23h–7h) to avoid disturbance. Configurable: start/end time and night brightness level |
| 6c | **OTA (Over-The-Air) firmware update** | `POST /api/ota` endpoint accepts a `.bin` file, writes via ESP32 `Update.h`. Reduces the need for a USB cable for updates |
| 6d | **Alert history** | Maintain a ring buffer of the last N received alerts (timestamp + text). Exposed via `GET /api/alerts/history`. Useful for debugging and auditing |
| 6e | **Slot scheduling by time of day** | Configure time windows in which a slot is displayed (e.g.: quotes only 9h–18h on weekdays). Requires only one extra field per slot and a check in `slotRotationTick()` |
| 6f | **Soft reboot via panel** | "Restart device" button in the Network/API tab that calls `POST /api/restart`; triggers `scheduleRestart(1500)`. Useful after complex configuration changes |

### Todo List

Each item 6a–6f should be treated as an independent sub-task when approved for implementation:

- **6a** — Modify only `web_page.h`: add a preview div in the Alert tab with JavaScript tag-resolution logic.
- **6b** — `config.h` (constants), `globals.h/cpp` (cfgNightBrightnessEnabled, cfgNightStart, cfgNightEnd, cfgNightBrightness), `persistence.cpp`, `display.cpp` (check in `displayTick()`), `web_page.h` (controls in Clock tab), `web_routes.cpp`.
- **6c** — `web_routes.cpp` (OTA endpoint with `Update.h`), `web_page.h` (upload button in API tab or new tab).
- **6d** — `globals.h/cpp` (alert ring buffer), `web_routes.cpp` (`GET /api/alerts/history`), `web_page.h` (display in Alert tab).
- **6e** — `config.h` (scheduling fields), `globals.h/cpp`, `persistence.cpp`, `display.cpp` (`slotRotationTick()`), `web_page.h`, `web_routes.cpp`.
- **6f** — `web_routes.cpp` (`POST /api/restart` calling `scheduleRestart(1500)`), `web_page.h` (button in Network tab).

### Relevant Context

- Items 6b and 6e depend on Sub-Task 1 (HH:MM:SS mode) being functional to avoid conflicts with `displayTick()` logic.
- Item 6c (`Update.h`) is built into the ESP32 Arduino core — no external libraries required.
- Item 6f (`scheduleRestart`) already exists in [`wifi_manager.cpp`](smart-matrix-clock-esp32/wifi_manager.cpp) — simply expose it via an HTTP route.

---

## Recommended Implementation Order

```
Sub-Task 1 (HH:MM:SS)          ← high urgency, isolated, no dependencies
    └── Sub-Task 2 (Icons)      ← medium urgency, depends on text_encoding
Sub-Task 6f (Restart button)   ← trivial, implement alongside ST2 or after
Sub-Task 3 (Web Basic Auth)    ← low urgency, required before ST5
Sub-Task 4 (UI i18n)           ← low urgency, independent
Sub-Task 5 (WiFi Scan)         ← low urgency
Sub-Task 6 remaining (6a-6e)   ← parallel with Phases 4 and 5 of original plan
```

---

## Compatibility with Phases 4 and 5

This plan does not conflict with Phases 4 (Weather) and 5 (Quotes) from [`docs/implementation-plan.md`](docs/implementation-plan.md). The sub-tasks described here modify:
- **Display** (Sub-Task 1): only the colon-blink block — Phase 4 additions are in `slotRotationTick()` which does not exist yet.
- **text_encoding** (Sub-Task 2): function addition, without modifying existing ones.
- **web_routes** (Sub-Tasks 3, 5): route and middleware additions — does not alter existing handlers.
- **web_page** (Sub-Tasks 2, 3, 4, 5): UI modifications do not affect firmware.

Phases 4 and 5 can be implemented in parallel or after this plan, without conflict.
