# Clock 12h / AM-PM Plan

## Overview

Add support for 12-hour clock format to the ESP32 matrix clock. The feature
consists of two orthogonal but related additions:

- **`cfgHour12`** — a boolean config toggle that switches the hour display
  between 24h (`14:35`) and 12h (`02:35`) in all existing clock modes.
- **`CLOCK_MODE_12H_AMPM`** — a new clock mode that renders `HH:MM` in 12h
  format and overlays a small `A` or `P` indicator using `_writeSmallDigit()`
  (same mechanism as the HH:MM:SS mode's seconds overlay).

Both settings are independent: `cfgHour12` affects how the hour digit is
computed; the AM/PM overlay is only active in the new mode.

The new mode value is `2`, extending the existing `0` (HH:MM) / `1`
(HH:MM:SS) range. The `cfgHour12` boolean is a separate NVS key.

### Design constraints

- Display is 32 physical columns. `HH:MM` in 12h occupies 17–26 px; the
  `A`/`P` small glyph (3 px) always fits in the remaining space.
- `_writeSmallDigit()` already handles the FC16_HW column-direction
  inversion; reuse it directly for the `A`/`P` glyph (both chars are in the
  `_dateSmallFont` table whose range is ASCII 32–90).
- The existing toggle in the web panel (`cfg-clock-mode` checkbox) becomes
  a 3-way selector: HH:MM (0) / HH:MM:SS (1) / 12h+AM/PM (2). Existing
  checkbox logic is replaced by a `<select>`.
- `cfgHour12` is also exposed as a separate toggle for users who only want
  plain 12h without the AM/PM overlay.
- NVS key lengths must stay ≤ 15 characters (ESP32 `Preferences` limit).

---

## Sub-Tasks

### ST-1 — Firmware: new constants and globals

**Intent:** Define the new mode constant and `cfgHour12` global so all other
sub-tasks have the symbols they depend on.

**Expected Outcomes:**
- `CLOCK_MODE_12H_AMPM = 2` constant available in `config.h`.
- `NVS_KEY_HOUR12` key defined in `config.h` (≤ 15 chars).
- `cfgHour12` extern bool declared in `globals.h` and defined in
  `globals.cpp` with default `false`.

**Todo List:**
1. In `config.h`: add `#define CLOCK_MODE_12H_AMPM 2` after line 78.
2. In `config.h`: add `#define NVS_KEY_HOUR12 "hour12"` in the NVS keys
   section.
3. In `globals.h`: add `extern bool cfgHour12;` near `cfgClockMode`.
4. In `globals.cpp`: add `bool cfgHour12 = false;`.

**Relevant Context:**
- [`config.h:76-81`](../smart-matrix-clock-esp32/config.h:76)
- [`globals.h:78`](../smart-matrix-clock-esp32/globals.h:78)
- [`globals.cpp:55`](../smart-matrix-clock-esp32/globals.cpp:55)

**Status:** [ ] pending

---

### ST-2 — Firmware: persistence (load / save / reset)

**Intent:** Persist `cfgHour12` and update the `cfgClockMode` range check so
the new mode 2 is accepted.

**Expected Outcomes:**
- `loadConfig()` reads `cfgHour12` from NVS; defaults to `false`.
- `saveConfig()` writes `cfgHour12` to NVS.
- `cfgClockMode` validation upper bound raised from `CLOCK_MODE_HHMMSS` (1)
  to `CLOCK_MODE_12H_AMPM` (2).
- `factoryReset()` resets `cfgHour12 = false`.

**Todo List:**
1. In `persistence.cpp` `loadConfig()`: add
   `cfgHour12 = _prefs.getBool(NVS_KEY_HOUR12, false);`
   after the `cfgClockMode` load line.
2. Change the range guard: `if (cfgClockMode > CLOCK_MODE_12H_AMPM)`.
3. In `saveConfig()`: add `_prefs.putBool(NVS_KEY_HOUR12, cfgHour12);`.
4. In `factoryReset()`: add `cfgHour12 = false;`.

**Relevant Context:**
- [`persistence.cpp:80-81`](../smart-matrix-clock-esp32/persistence.cpp:80)
- [`persistence.cpp:133`](../smart-matrix-clock-esp32/persistence.cpp:133)
- [`persistence.cpp:200`](../smart-matrix-clock-esp32/persistence.cpp:200)

**Status:** [ ] pending

---

### ST-3 — Firmware: display rendering

**Intent:** Add a helper `_renderHHMM12hAMPM()` that renders `HH:MM` in 12h
format with a small `A` or `P` glyph overlay, and integrate it into
`displayTick()`. Also apply `cfgHour12` to the existing HH:MM and HH:MM:SS
modes.

**Expected Outcomes:**
- New `static void _renderHHMM12hAMPM(struct tm& t, bool colonVisible)`
  added in `display.cpp`, placed near `_renderHHMMSS()`.
- `CLOCK_MODE_12H_AMPM` branch added in the `displayTick()` mode switch.
- `CLOCK_MODE_HHMM` and `CLOCK_MODE_HHMMSS` branches apply `cfgHour12` when
  formatting the hour digit (`t.tm_hour % 12 || 12` when true).
- Before-NTP-sync path in the new mode shows `--:--` with no AM/PM overlay
  (same as existing unsync'd handling).

**Implementation notes:**
- Hour conversion: `int h = cfgHour12 ? (t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12) : t.tm_hour;`
- AM/PM char: `char ampm = (t.tm_hour < 12) ? 'A' : 'P';`
- After `_display.print(hmBuf)` (PA_LEFT), compute `hmWidth` via
  `getTextColumns()`, then call `_writeSmallDigit(hmWidth + 1, ampm)`.
  The `A` and `P` glyphs are both in `_dateSmallFont` (range 32–90).
- If `hmWidth + 1 + SS_DIGIT_CELL_PX > DISPLAY_WIDTH_PX`, pack with no gap:
  `_writeSmallDigit(DISPLAY_WIDTH_PX - SS_DIGIT_CELL_PX, ampm)`.

**Todo List:**
1. Add `_renderHHMM12hAMPM()` static function after `_renderHHMMSS()`.
2. In `displayTick()`, extend the mode check to add an
   `else if (cfgClockMode == CLOCK_MODE_12H_AMPM)` branch calling
   `_renderHHMM12hAMPM()` on the same 500 ms blink timer.
3. In the existing `CLOCK_MODE_HHMM` `snprintf`, replace `timeinfo.tm_hour`
   with `cfgHour12 ? (timeinfo.tm_hour % 12 == 0 ? 12 : timeinfo.tm_hour % 12) : timeinfo.tm_hour`.
4. Apply the same conversion in `_renderHHMMSS()` for the HH part.

**Relevant Context:**
- [`display.cpp:358-373`](../smart-matrix-clock-esp32/display.cpp:358)
  — `_renderHHMMSS()` reference implementation
- [`display.cpp:310-334`](../smart-matrix-clock-esp32/display.cpp:310)
  — `_writeSmallDigit()`
- [`display.cpp:665-717`](../smart-matrix-clock-esp32/display.cpp:665)
  — `displayTick()` mode switch
- [`date_font.h:33`](../smart-matrix-clock-esp32/date_font.h:33)
  — font range 32–90 confirms `A` (65) and `P` (80) are available

**Status:** [ ] pending

---

### ST-4 — Firmware: REST API

**Intent:** Expose `cfgHour12` and the extended `clock_mode` range through
the existing `/api/config` GET and POST endpoints.

**Expected Outcomes:**
- `GET /api/config` response includes `"hour12": true/false`.
- `POST /api/config` accepts `{"hour12": <bool>}` and sets `cfgHour12`; sets
  `clockModeChangePending = true` so the display redraws.
- `POST /api/config` `clock_mode` validation upper bound raised to `2`.
- `GET /api/status` `time_str` shows 12h value when
  `cfgHour12 == true` (already used in web panel live preview).
- Error message for out-of-range `clock_mode` updated to say `0–2`.

**Todo List:**
1. In `web_routes.cpp` GET `/api/config` handler: add
   `doc["hour12"] = cfgHour12;` near `cfgClockMode`.
2. In POST `/api/config` handler: update `clock_mode` range guard to
   `v > 2`; update error string.
3. In POST `/api/config` handler: add `hour12` field handling — read bool,
   set `cfgHour12`, set `clockModeChangePending = true`, set `changed = true`.
4. In GET `/api/status` `time_str` formatting: apply 12h conversion when
   `cfgHour12 == true`.

**Relevant Context:**
- [`web_routes.cpp:60-87`](../smart-matrix-clock-esp32/web_routes.cpp:60)
  — `/api/status` handler
- [`web_routes.cpp:145`](../smart-matrix-clock-esp32/web_routes.cpp:145)
  — `/api/config` GET
- [`web_routes.cpp:261-266`](../smart-matrix-clock-esp32/web_routes.cpp:261)
  — `/api/config` POST `clock_mode` block

**Status:** [ ] pending

---

### ST-5 — Web panel: UI controls and I18N

**Intent:** Replace the single "Show seconds" checkbox with a `<select>`
dropdown for the three clock modes, and add a separate toggle for the 12h
option. Update all I18N keys for English and Portuguese.

**Expected Outcomes:**
- The "Clock mode" card now contains:
  - A `<select id="cfg-clock-mode">` with options:
    - `value="0"` → i18n key `clock.modeHHMM` ("HH:MM")
    - `value="1"` → i18n key `clock.modeHHMMSS` ("HH:MM:SS")
    - `value="2"` → i18n key `clock.mode12hAMPM` ("12h + AM/PM")
  - A toggle row for `<input type="checkbox" id="cfg-hour12"/>` labelled
    with i18n key `clock.hour12Label`.
  - A hint below the toggle using i18n key `clock.hour12Hint`.
- `loadConfig()` JS reads `c.clock_mode` (integer) to set the select value,
  and `c.hour12` (bool) to set the checkbox.
- `buildPayload()` JS sends `clock_mode: parseInt(val('cfg-clock-mode'))` and
  `hour12: checked('cfg-hour12')`.
- The live preview `addEventListener` on `cfg-clock-mode` (change) extends
  to handle mode 2 (shows `hh:mm A` or `hh:mm P` approximation based on
  current time).
- All new i18n keys added to both `en` and `pt` dictionaries in
  `web_page.cpp`.

**Todo List:**
1. In `web_page.cpp` HTML section: replace the checkbox + label for
   `cfg-clock-mode` with a `<select>` plus a new toggle row for `cfg-hour12`.
2. Update `en` I18N dictionary: add `clock.modeHHMM`, `clock.modeHHMMSS`,
   `clock.mode12hAMPM`, `clock.hour12Label`, `clock.hour12Hint`; remove or
   keep `clock.showSeconds` (keep for backward compat if preferred, but it
   is no longer used by any HTML element after the replacement).
3. Update `pt` I18N dictionary with matching Portuguese translations.
4. In the JS `loadConfig` function: update `cfg-clock-mode` to set
   `document.getElementById('cfg-clock-mode').value = String(c.clock_mode || 0)`;
   add `setChecked('cfg-hour12', c.hour12 || false)`.
5. In JS `buildPayload`: replace the `clock_mode` line to use `parseInt`.
   Add `hour12: checked('cfg-hour12')`.
6. Update the `cfg-clock-mode` live preview listener: handle the three
   option values.

**Relevant Context:**
- [`web_page.cpp:215-222`](../smart-matrix-clock-esp32/web_page.cpp:215)
  — existing Clock mode card HTML
- [`web_page.cpp:505-520`](../smart-matrix-clock-esp32/web_page.cpp:505)
  — `en` I18N dictionary (approximate line)
- [`web_page.cpp:600-615`](../smart-matrix-clock-esp32/web_page.cpp:600)
  — `pt` I18N dictionary (approximate line)
- [`web_page.cpp:841`](../smart-matrix-clock-esp32/web_page.cpp:841)
  — `loadConfig` JS for `cfg-clock-mode`
- [`web_page.cpp:936`](../smart-matrix-clock-esp32/web_page.cpp:936)
  — `buildPayload` JS for `clock_mode`
- [`web_page.cpp:1101`](../smart-matrix-clock-esp32/web_page.cpp:1101)
  — live preview listener

**Status:** [ ] pending
