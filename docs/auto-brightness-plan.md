# Auto Brightness by Time of Day — Implementation Plan

## Overview

Add configurable automatic brightness reduction by time of day (Sub-Task 6b from
`docs/enhancements-plan.md`). When enabled, the display brightness is automatically lowered
to a "night" level during a configurable time window (e.g. 23:00–07:00) and restored to
the configured day brightness outside that window.

**Scope:** 6 files — `config.h`, `globals.h`, `globals.cpp`, `persistence.cpp`,
`display.cpp`, `web_routes.cpp`, `web_page.cpp`.

**Non-goals:**
- No LDR/photosensor support.
- The per-message brightness override (`messageBrightness`) is unaffected.
- Factory reset resets auto-brightness to disabled (same as all new feature flags).

---

## Logic Summary

```
night window active?
  start ≤ end  (same day)   → now_min >= start AND now_min < end
  start > end  (crosses midnight) → now_min >= start OR  now_min < end
  start == end               → never active (feature disabled)

effective brightness = nightActive && cfgNightBrightnessEnabled
                       ? cfgNightBrightnessLevel
                       : currentBrightness   (day value set by user)
```

`displayTick()` runs the check every second (guarded by the same second-change
detection already in place for HH:MM). When night mode activates or deactivates,
`_display.setIntensity()` is called once — not on every tick.

---

## Sub-Tasks

---

### ST-1 — Constants and NVS keys (`config.h`)

**Intent:** Define all compile-time defaults and NVS key strings for the four new
config values. NVS keys must be ≤ 15 characters.

**Expected Outcomes:**
- Four `#define` constants for defaults.
- Four `#define` NVS key strings (≤ 15 chars each).

**Todo List:**
1. After the existing `NVS_KEY_SLOT3_SCHED_DAYS` block, add a new section
   `// ─── Auto brightness by time of day (Sub-Task 6b)`.
2. Add defaults:
   - `DEFAULT_NIGHT_BRIGHTNESS_ENABLED` → `false`
   - `DEFAULT_NIGHT_BRIGHTNESS_LEVEL` → `0`  (fully off at night)
   - `DEFAULT_NIGHT_START_MIN` → `1380`  (23:00 in minutes-of-day)
   - `DEFAULT_NIGHT_END_MIN` → `420`    (07:00 in minutes-of-day)
3. Add NVS keys:
   - `NVS_KEY_NIGHT_BRI_EN` → `"night_bri_en"`   (12 chars ✓)
   - `NVS_KEY_NIGHT_BRI_LVL` → `"night_bri_lvl"` (13 chars ✓)
   - `NVS_KEY_NIGHT_START` → `"night_start"`     (11 chars ✓)
   - `NVS_KEY_NIGHT_END` → `"night_end"`         (9 chars ✓)

**Relevant Context:**
- [`smart-matrix-clock-esp32/config.h`](smart-matrix-clock-esp32/config.h:175) — existing NVS key block ends at line 181.
- Key length rule: ESP32 `Preferences` keys ≤ 15 chars (from AGENTS.md).

**Status:** [ ] pending

---

### ST-2 — Globals declaration and initialisation (`globals.h` / `globals.cpp`)

**Intent:** Expose the four new config variables as `extern` globals so all
modules can read them, and initialise them to their compile-time defaults.

**Expected Outcomes:**
- `globals.h` declares `cfgNightBrightnessEnabled`, `cfgNightBrightnessLevel`,
  `cfgNightStartMin`, `cfgNightEndMin`.
- `globals.cpp` defines and initialises them.

**Todo List:**
1. In [`globals.h`](smart-matrix-clock-esp32/globals.h:78), add a new section after the
   `cfgClockMode` line:
   ```
   // ─── Auto brightness by time of day (Sub-Task 6b) ─────────────────────────
   extern bool     cfgNightBrightnessEnabled; // true = auto dimming active
   extern uint8_t  cfgNightBrightnessLevel;   // brightness during night window (0–15)
   extern uint16_t cfgNightStartMin;          // night window start, minutes-of-day (0–1439)
   extern uint16_t cfgNightEndMin;            // night window end, minutes-of-day (0–1439); start==end means disabled
   ```
2. In [`globals.cpp`](smart-matrix-clock-esp32/globals.cpp), add matching definitions
   with initial values from the new `config.h` defaults.

**Relevant Context:**
- [`smart-matrix-clock-esp32/globals.h`](smart-matrix-clock-esp32/globals.h:78) — `cfgClockMode` is at line 78.
- [`smart-matrix-clock-esp32/globals.cpp`](smart-matrix-clock-esp32/globals.cpp) — follow the same initialisation pattern as other cfg* variables.

**Status:** [ ] pending

---

### ST-3 — Persistence: load, save, and factory reset (`persistence.cpp`)

**Intent:** Persist the four new config variables in NVS so settings survive
power cycles, and reset them during factory reset.

**Expected Outcomes:**
- `loadConfig()` reads all four values from NVS with fallback to defaults.
- `saveConfig()` writes all four values.
- `factoryReset()` resets all four RAM globals to defaults (NVS is cleared by
  `_prefs.clear()` already).

**Todo List:**
1. In `loadConfig()`, after the slot-scheduling block (line 102), add:
   ```cpp
   // ── Auto brightness (Sub-Task 6b) ──────────────────────────────────────────
   cfgNightBrightnessEnabled = _prefs.getBool(NVS_KEY_NIGHT_BRI_EN,  DEFAULT_NIGHT_BRIGHTNESS_ENABLED);
   cfgNightBrightnessLevel   = _prefs.getUChar(NVS_KEY_NIGHT_BRI_LVL, DEFAULT_NIGHT_BRIGHTNESS_LEVEL);
   cfgNightStartMin          = _prefs.getUShort(NVS_KEY_NIGHT_START,  DEFAULT_NIGHT_START_MIN);
   cfgNightEndMin            = _prefs.getUShort(NVS_KEY_NIGHT_END,    DEFAULT_NIGHT_END_MIN);
   ```
2. In `saveConfig()`, after the slot-scheduling block (line 153), add the four
   matching `_prefs.put*()` calls.
3. In `factoryReset()`, after the existing reset block, add the four global
   assignments restoring defaults.

**Relevant Context:**
- [`smart-matrix-clock-esp32/persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp:96) — slot-scheduling block at lines 96–102.
- [`smart-matrix-clock-esp32/persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp:147) — saveConfig slot-scheduling block at lines 147–153.
- [`smart-matrix-clock-esp32/persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp:172) — factoryReset at lines 172–190.

**Status:** [ ] pending

---

### ST-4 — Auto-brightness check in `displayTick()` (`display.cpp`)

**Intent:** Apply the night/day brightness automatically on every second tick
inside `displayTick()`, without blocking and without redundant `setIntensity()`
calls.

**Expected Outcomes:**
- A static `_lastNightActive` flag tracks whether night mode was active last tick.
- Each second, the current minutes-of-day is computed from `localtime()`.
- If `cfgNightBrightnessEnabled` is true and night window is active, and the state
  changed, `_display.setIntensity(cfgNightBrightnessLevel)` is called.
- If night window ends, `_display.setIntensity(currentBrightness)` is called.
- During an active message scroll (`messageBrightness >= 0`), auto-brightness does
  NOT override — the restore is handled by `_restoreBrightness()` already.

**Placement:** Insert the auto-brightness block at the **top** of `displayTick()`,
before the `clockModeChangePending` check, so it runs on every tick cycle.

**Logic pseudocode:**
```
static bool _lastNightActive = false;
if (cfgNightBrightnessEnabled && ntpSynced) {
    time_t now_t = time(nullptr);
    struct tm* tm = localtime(&now_t);
    uint16_t nowMin = tm->tm_hour * 60 + tm->tm_min;
    bool nightActive;
    if (cfgNightStartMin == cfgNightEndMin) {
        nightActive = false;  // disabled sentinel
    } else if (cfgNightStartMin < cfgNightEndMin) {
        nightActive = (nowMin >= cfgNightStartMin && nowMin < cfgNightEndMin);
    } else {
        nightActive = (nowMin >= cfgNightStartMin || nowMin < cfgNightEndMin);
    }
    if (nightActive != _lastNightActive) {
        _lastNightActive = nightActive;
        if (messageBrightness < 0) {   // don't override active message override
            _display.setIntensity(nightActive ? cfgNightBrightnessLevel : currentBrightness);
        }
    }
}
```

**Todo List:**
1. Add `static bool _lastNightActive = false;` as a file-scope static in
   `display.cpp` near the other `_last*` statics.
2. Insert the auto-brightness check block at the very start of `displayTick()`,
   before line 529 (`clockModeChangePending` check).
3. In `setBrightness()` (lines 720–723): after updating `currentBrightness` and
   calling `_display.setIntensity()`, also reset `_lastNightActive = false` so
   that the next `displayTick()` re-evaluates and reapplies night dimming if
   the window is still active. This ensures manual brightness changes are
   immediately visible and night mode re-asserts on the next second.

**Relevant Context:**
- [`smart-matrix-clock-esp32/display.cpp`](smart-matrix-clock-esp32/display.cpp:526) — `displayTick()` starts at line 526.
- [`smart-matrix-clock-esp32/display.cpp`](smart-matrix-clock-esp32/display.cpp:720) — `setBrightness()` at lines 720–723.
- Architecture rule: `loop()` is single-threaded and must never block —
  `time(nullptr)` + `localtime()` are non-blocking.

**Status:** [ ] pending

---

### ST-5 — REST API: expose and accept the 4 new fields (`web_routes.cpp`)

**Intent:** The web panel must be able to read and write the auto-brightness
config via the existing `/api/config` GET/POST endpoints.

**Expected Outcomes:**
- `GET /api/config` returns `night_brightness_enabled`, `night_brightness_level`,
  `night_start_min`, `night_end_min`.
- `POST /api/config` accepts and validates those four fields, applying them
  immediately and marking `changed = true`.

**Todo List:**
1. In `_handleGetConfig()`, after the `doc["clock_mode"]` line (line 145), add
   the four new JSON fields.
2. In `_handlePostConfig()`, after the `brightness` block (~line 192), add a new
   block for each of the four fields:
   - `night_brightness_enabled` — bool, no validation needed.
   - `night_brightness_level` — int, range 0–15.
   - `night_start_min` — int, range 0–1439.
   - `night_end_min` — int, range 0–1439.
   - After setting all four, call `_lastNightActive = false` indirectly via
     `setBrightness(currentBrightness)` OR by simply allowing `displayTick()`
     to re-evaluate on the next second. No explicit display call needed here —
     `displayTick()` will pick it up within ≤1 second.

**Relevant Context:**
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp:134) — `_handleGetConfig()`.
- [`smart-matrix-clock-esp32/web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp:186) — brightness POST block.

**Status:** [ ] pending

---

### ST-6 — Web panel UI and I18N (`web_page.cpp`)

**Intent:** Add a "Night Mode" sub-section inside the existing "Display" card in
the Clock tab. The user can toggle auto-dimming, set start/end time, and set the
night brightness level. All labels are translated in both EN and PT I18N
dictionaries.

**Expected Outcomes:**
- A checkbox to enable/disable auto-brightness.
- Two `<input type="time">` fields for start and end of the night window.
- A range slider (0–15) for the night brightness level, identical in style to
  the existing day brightness slider.
- Values are loaded from `/api/config` on page load and saved via the existing
  "Save settings" button.
- Both EN and PT I18N keys added.

**HTML structure (inside the existing Display card, after the day brightness
slider block, before the scroll speed block):**
```html
<div class="field" style="margin-top:14px">
  <label>
    <input type="checkbox" id="cfg-night-bri-en"/>
    <span data-i18n="clock.nightBrightness">Night mode (auto dim)</span>
  </label>
</div>
<div id="night-bri-settings" style="display:none">
  <div class="range-field" style="margin-top:8px">
    <label><span data-i18n="clock.nightBrightnessLevel">Night brightness</span>:
      <span id="night-bri-val">0</span> / 15</label>
    <div class="range-row">
      <span style="font-size:12px;color:#8b949e">0</span>
      <input id="cfg-night-bri-level" type="range" min="0" max="15" value="0"/>
      <span style="font-size:12px;color:#8b949e">15</span>
      <span class="range-val" id="night-bri-val-badge">0</span>
    </div>
  </div>
  <div class="field" style="margin-top:8px">
    <label data-i18n="clock.nightStart">Night starts at</label>
    <input id="cfg-night-start" type="time" value="23:00"/>
  </div>
  <div class="field" style="margin-top:8px">
    <label data-i18n="clock.nightEnd">Night ends at</label>
    <input id="cfg-night-end" type="time" value="07:00"/>
  </div>
</div>
```

**JS changes (in the existing inline `<script>`):**
- Toggle visibility of `#night-bri-settings` when `#cfg-night-bri-en` changes.
- Live update of `#night-bri-val` / `#night-bri-val-badge` on range input.
- In `loadConfig()`: populate all four fields from API response. Show/hide
  `#night-bri-settings` based on `night_brightness_enabled`.
- Helper: convert minutes-of-day (integer) → `"HH:MM"` string for the time
  inputs; and `"HH:MM"` → minutes-of-day integer for the POST payload.
- In the "Save settings" click handler: add the four fields to the POST body.

**I18N keys to add (EN and PT):**

| Key | EN | PT |
|-----|----|----|
| `clock.nightBrightness` | `Night mode (auto dim)` | `Modo noturno (auto brilho)` |
| `clock.nightBrightnessLevel` | `Night brightness` | `Brilho noturno` |
| `clock.nightStart` | `Night starts at` | `Anoitece às` |
| `clock.nightEnd` | `Night ends at` | `Amanhece às` |

**Todo List:**
1. Insert the HTML block inside the Display card in `web_page.cpp`, after the
   day brightness slider (after line 176, before line 177 where scroll speed starts).
2. Add the toggle visibility JS handler for `#cfg-night-bri-en` checkbox.
3. Add the range live-update handler for `#cfg-night-bri-level`.
4. Add min→time and time→min helpers as small inline JS functions.
5. Populate form fields in the `loadConfig()` JS callback.
6. Add the four fields to the POST body in the "Save settings" handler.
7. Add 4 EN keys to the `I18N.en` dictionary (after `clock.brightness` at line 516).
8. Add 4 PT keys to the `I18N.pt` dictionary (after `clock.brightness` at line 611).

**Relevant Context:**
- [`smart-matrix-clock-esp32/web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:166) — Display card HTML at lines 166–194.
- [`smart-matrix-clock-esp32/web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:499) — I18N dictionary at line 499.
- [`smart-matrix-clock-esp32/web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:781) — range input live update at lines 781–783.
- [`smart-matrix-clock-esp32/web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:848) — loadConfig JS at lines 848–851.

**Status:** [ ] pending

---

## Implementation Order

```
ST-1 (config.h)
  └── ST-2 (globals)
        ├── ST-3 (persistence)
        ├── ST-4 (display logic)
        └── ST-5 (web_routes)
              └── ST-6 (web_page UI)
```

Each sub-task depends on the previous one compiling cleanly before proceeding.
Run `arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32`
after every sub-task.
