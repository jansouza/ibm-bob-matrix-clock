# Countdowns & Timers — Implementation Plan

## Overview

Add a **Countdown slot (slot 4)** to the LED matrix display rotation. Users configure up to 4 named countdowns (e.g. "New Year", "Birthday") via the web panel. The display shows a live scrolling string such as `New Year 3d 14h 20m` or `Meeting 45m 10s`. When a countdown reaches zero, a one-shot bell message fires (`[bell] New Year!`) and the entry is automatically disabled. The slot participates in the standard rotation infrastructure — it has its own enable flag, display interval, and schedule (time-of-day + weekday mask).

**Scope:** Countdowns only (count-down to a target date/time). Count-up timers are out of scope.

**Max entries:** 4 (`COUNTDOWN_MAX_ENTRIES`).

**Slot index:** 4 (`SLOT_COUNTDOWN`).

---

## Sub-Task A — `config.h`: constants and NVS keys

**Status:** `[ ] pending`

### Intent

Declare all compile-time constants for the new slot in the single source-of-truth header. Nothing in the firmware changes behaviour until these constants are referenced by the other sub-tasks.

### Expected Outcomes

- `SLOT_COUNTDOWN 4` constant available firmware-wide.
- `COUNTDOWN_MAX_ENTRIES 4` caps the entry array size.
- Display defaults (`COUNTDOWN_DISPLAY_DEFAULT_MS`, min/max) follow the same pattern as weather/quotes.
- NVS keys for the slot's enable flag, display interval, scheduling, and per-entry data are defined and all ≤ 15 characters.
- Label max length constant defined (`COUNTDOWN_LABEL_MAX`).

### Todo List

1. Add a `// ─── Countdown slot ───` section after the quotes NVS keys block (after line 184).
2. Add `#define SLOT_COUNTDOWN 4`.
3. Add `#define COUNTDOWN_MAX_ENTRIES 4`.
4. Add display interval defaults/bounds mirroring weather/quotes: `COUNTDOWN_DISPLAY_DEFAULT_MS 30000UL`, `COUNTDOWN_DISPLAY_MIN_MS 5000UL`, `COUNTDOWN_DISPLAY_MAX_MS 300000UL`.
5. Add `#define COUNTDOWN_LABEL_MAX 24` (label string including null terminator).
6. Add NVS keys for slot-level config (following the `s4_*` pattern, all ≤ 15 chars):
   - `NVS_KEY_SLOT4_EN   "slot4_en"` (8 chars)
   - `NVS_KEY_SLOT4_MS   "slot4_ms"` (8 chars)
   - `NVS_KEY_SLOT4_SCHED_START "s4_sch_st"` (9 chars)
   - `NVS_KEY_SLOT4_SCHED_END   "s4_sch_en"` (9 chars)
   - `NVS_KEY_SLOT4_SCHED_DAYS  "s4_sch_dy"` (9 chars)
7. Add NVS keys for per-entry data. Pattern: `"cd{i}_lbl"`, `"cd{i}_ts"` (Unix timestamp as uint32), `"cd{i}_en"` for i = 0–3 (all ≤ 15 chars).

### Relevant Context

- All existing NVS key patterns are in [`config.h`](smart-matrix-clock-esp32/config.h:97).
- Slot scheduling NVS key pattern: `"s2_sch_st"`, `"s3_sch_st"` — follow same prefix `"s4_sch_*"`.
- Per-entry keys: `"cd0_lbl"` (7), `"cd0_ts"` (6), `"cd0_en"` (6) — well within 15-char limit.
- The test `test_persistence_language.cpp` validates that all `NVS_KEY_*` string literals are ≤ 15 chars — new keys will be covered automatically if they follow the `#define NVS_KEY_*` naming convention checked there.

---

## Sub-Task B — `globals.h/cpp`: data structures and array expansion

**Status:** `[ ] pending`

### Intent

Define the `CountdownEntry` struct and expand all slot arrays from size 4 to size 5 to accommodate slot index 4. This is the change that makes the rest of the firmware aware that a fifth slot exists.

### Expected Outcomes

- `CountdownEntry` struct declared in `globals.h` with fields: `char label[COUNTDOWN_LABEL_MAX]`, `uint32_t targetEpoch`, `bool enabled`.
- `CountdownEntry countdownEntries[COUNTDOWN_MAX_ENTRIES]` array declared (`extern`) in `globals.h` and defined in `globals.cpp`.
- All five slot arrays resized to `[5]` with index 4 initialised: `slotEnabled[4] = false`, `slotIntervalMs[4] = COUNTDOWN_DISPLAY_DEFAULT_MS`, schedule arrays index 4 initialised to always-active defaults (`start == end == 0`, `daysMask == SLOT_SCHEDULE_ALL_DAYS`).
- `_slotLastShownMs[5]` static array in `display.cpp` also expanded (see Sub-Task D).

### Todo List

1. In `globals.h`, add `CountdownEntry` struct definition (after the `QuoteCache` struct, before extern declarations).
2. In `globals.h`, add `extern CountdownEntry countdownEntries[];` declaration.
3. In `globals.cpp`, define `CountdownEntry countdownEntries[COUNTDOWN_MAX_ENTRIES]` initialised to all zeros/empty.
4. In `globals.cpp`, change all five slot array dimensions from `[4]` to `[5]` and add index-4 initialiser values.

### Relevant Context

- Existing cache structs are in [`globals.h`](smart-matrix-clock-esp32/globals.h) — `WeatherCache` and `QuoteCache` are the models.
- Array initializers are in [`globals.cpp`](smart-matrix-clock-esp32/globals.cpp:37) lines 37–44.
- `targetEpoch` is `uint32_t` (Unix timestamp — valid through year 2106, sufficient for this use case).
- `label` is Latin-1, same encoding as `messageText[]`, so `expandIconTags()` can be applied if needed.

---

## Sub-Task C — `persistence.h/cpp`: load, save, and factory reset

**Status:** `[ ] pending`

### Intent

Persist the countdown slot's enable/interval/schedule config and each entry's label, target epoch, and enabled flag in NVS. Entries survive reboots. Factory reset clears all entries and resets slot config to defaults.

### Expected Outcomes

- `loadConfig()` reads slot 4's enable, interval, and schedule from NVS; reads up to `COUNTDOWN_MAX_ENTRIES` entries (label as string, targetEpoch as uint32, enabled as bool).
- `saveConfig()` writes the same fields back.
- `factoryReset()` resets slot 4 to `enabled=false`, `intervalMs=COUNTDOWN_DISPLAY_DEFAULT_MS`, schedule to always-active, and all entries to empty/disabled.

### Todo List

1. In `loadConfig()`, after the slot 3 schedule block, add slot 4 load: `slotEnabled[4]`, `slotIntervalMs[4]`, and schedule keys `NVS_KEY_SLOT4_SCHED_*`.
2. In `loadConfig()`, add a loop `for (int i = 0; i < COUNTDOWN_MAX_ENTRIES; i++)` that reads `cd{i}_lbl` (string), `cd{i}_ts` (UInt), `cd{i}_en` (Bool) into `countdownEntries[i]`.
3. In `saveConfig()`, add the mirroring writes for slot 4 config and all entry keys.
4. In `factoryReset()`, reset slot 4 config and zero-out all `countdownEntries[]` (label = `""`, `targetEpoch = 0`, `enabled = false`).

### Relevant Context

- Pattern to follow: [`persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp) — slot 3 blocks at lines 96–102 (load) and 154–159 (save) and 230–233 (reset).
- NVS key construction for per-entry data: build key strings at runtime with `snprintf(key, sizeof(key), "cd%d_lbl", i)` — or use compile-time macros if preferred; runtime snprintf is simpler for indexed keys.
- `_prefs.getString(key, countdownEntries[i].label, COUNTDOWN_LABEL_MAX)` — use the `getString(key, buf, len)` overload, not `getStr` which returns a `String` object.

---

## Sub-Task D — `display.cpp`: countdown slot rendering

**Status:** `[ ] pending`

### Intent

Add the countdown slot to the display rotation. When slot 4 is due, build a scroll string for each active entry in sequence (one per rotation turn, cycling through them), check for zero-crossing on each tick and fire the bell message when reached.

### Expected Outcomes

- `displayBegin()` initialises `_slotLastShownMs[4]`.
- `_startSlotScroll()` has an `else if (slot == 4)` branch that calls `_buildCountdownString()` for the current active entry index.
- `_buildCountdownString()` formats the remaining time as `<label> Xd Xh Xm` when ≥ 1 hour remains, or `<label> Xm Xs` when < 1 hour, or fires the bell message and disables the entry when the target has passed.
- `_slotRotationTick()` includes slot 4 in the `slotDue` logic using the same `slotEnabled[4]` + `_slotInSchedule(4)` + interval check pattern as slots 2 and 3.
- `_nextCountdownEntry()` helper advances a static `_countdownActiveIdx` to the next enabled entry; if no entries are enabled, returns -1 and the slot is skipped.
- The `_slotLastShownMs[]` static array is expanded to size 5.

### Todo List

1. Expand `static uint32_t _slotLastShownMs[4]` to `[5]` and add `_slotLastShownMs[4] = millis()` in `displayBegin()`.
2. Add static `_countdownActiveIdx` (0–3) to track which entry is shown next.
3. Add `_buildCountdownString(char* buf, size_t len)` static function:
   - Iterates from `_countdownActiveIdx` to find the next enabled entry.
   - Computes `int32_t remaining = (int32_t)entry.targetEpoch - (int32_t)time(NULL)`.
   - If `remaining <= 0`: call `_fireCountdownBell(i)` (injects bell message, sets `entry.enabled = false`, calls `saveConfig()`), advance index, try next entry.
   - If `remaining > 0`: format string; advance `_countdownActiveIdx` for the next call.
   - Returns `true` if a string was built, `false` if no active entries remain.
4. Add `_fireCountdownBell(uint8_t idx)` static function: builds `"[bell] <label>!"`, calls `utf8ToLatin1` + `expandIconTags` and writes to `messageText[]`, sets `messagePending = true`.
5. Add `else if (slot == 4)` branch in `_startSlotScroll()` calling `_buildCountdownString()`.
6. Add `slot4Due` check in `_slotRotationTick()` and integrate into the tie-breaking logic (extend the existing `slot2Due / slot3Due` pattern to three candidates).

### Relevant Context

- `_startSlotScroll()` and `_slotRotationTick()` are in [`display.cpp`](smart-matrix-clock-esp32/display.cpp) around lines 430–515.
- `_slotInSchedule()` at line 456 is directly reusable for slot 4 — no changes needed.
- Message injection pattern: set `messagePending = true` and write to `messageText[]` (Latin-1, `MAX_MESSAGE_LEN`). Order: `utf8ToLatin1` first, then `expandIconTags`. Both in [`text_encoding.h`](smart-matrix-clock-esp32/text_encoding.h).
- `time(NULL)` returns UTC seconds since epoch — same units as `targetEpoch`.
- `saveConfig()` must be called after disabling a fired entry (persists the `enabled=false` state so it doesn't fire again after reboot).
- `_buildWeatherString()` / `_buildQuotesString()` are models for the string builder pattern.
- Tie-breaking for three slots: pick the one with the largest `(now - _slotLastShownMs[slot]) - slotIntervalMs[slot]` overdue value.

---

## Sub-Task E — `web_routes.cpp`: REST API for countdown config and entries

**Status:** `[ ] pending`

### Intent

Expose countdown slot config (enable, display interval, schedule) through the existing `POST /api/config` and `GET /api/config` handlers, and expose entry CRUD through three new routes: `GET /api/countdowns`, `POST /api/countdowns`, and `DELETE /api/countdowns/{index}`.

### Expected Outcomes

- `GET /api/config` response includes `countdowns_enabled`, `countdowns_display_ms`, `countdowns_sched_start`, `countdowns_sched_end`, `countdowns_sched_days`.
- `POST /api/config` accepts and validates those same fields (interval bounds: `COUNTDOWN_DISPLAY_MIN_MS`–`COUNTDOWN_DISPLAY_MAX_MS`; days mask: 0–127).
- `GET /api/countdowns` returns a JSON array of up to 4 entries: `[{index, label, target_epoch, enabled}, ...]`.
- `POST /api/countdowns` accepts `{index, label, target_epoch, enabled}` — validates `index` 0–3, label length ≤ `COUNTDOWN_LABEL_MAX-1`, `target_epoch > 0`; writes to `countdownEntries[]`; calls `saveConfig()`.
- `DELETE /api/countdowns/{index}` clears the entry at that index; calls `saveConfig()`.
- All handlers are zero-latency (no display/network I/O — only set globals and save NVS).

### Todo List

1. In `_handleGetConfig()`, add five new fields for slot 4 config (follow the slot 3 quotes pattern at the end of the existing block).
2. In `_handlePostConfig()`, add validation + assignment for the five slot 4 config fields; call `saveConfig()` on change.
3. Add `_handleGetCountdowns()`: builds a JSON array from `countdownEntries[]`; sends 200.
4. Add `_handlePostCountdown()` (body handler): parses `{index, label, target_epoch, enabled}`; validates; writes to `countdownEntries[index]`; calls `saveConfig()`; sends `_sendOk()`.
5. Add `_handleDeleteCountdown()`: extracts `{index}` from URL param; clears entry; calls `saveConfig()`; sends `_sendOk()`.
6. In `webRoutesBegin()`, register the three new routes:
   - `server.on("/api/countdowns", HTTP_GET, _handleGetCountdowns)`
   - `server.on("/api/countdowns", HTTP_POST, ...)` with body handler `_handlePostCountdown`
   - `server.on("/api/countdowns/{index}", HTTP_DELETE, _handleDeleteCountdown)`

### Relevant Context

- POST body handler registration pattern (two-argument `server.on`): see [`web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp) around line 657 for the quotes POST registration.
- URL path parameter extraction: `req->pathArg(0)` — requires `{index}` in the route pattern; see the wifi scan handler for the pattern.
- Input validation for label: use `strnlen(label, COUNTDOWN_LABEL_MAX)` — reject if length ≥ `COUNTDOWN_LABEL_MAX`.
- `target_epoch` is transmitted as a JSON number (uint32). On the JS side `Math.floor(Date.getTime() / 1000)` provides the value.

---

## Sub-Task F — `web_page.cpp`: Countdowns tab in the web panel

**Status:** `[ ] pending`

### Intent

Add a **Countdowns** tab to the web panel where the user can view, add, enable/disable, and delete countdown entries, and configure the slot's display interval and schedule.

### Expected Outcomes

- A **Countdowns** tab button appears in the tab bar (after Quotes).
- The tab panel shows:
  - Slot enable checkbox + display interval (seconds) input.
  - Schedule fields (start time, end time, weekday mask) — reusing the same HTML/JS helpers already used by Weather and Quotes.
  - Entry list: up to 4 rows, each showing label, target datetime, enabled toggle, delete button.
  - Add-entry form: label text input, datetime-local input, Save button.
- `loadConfig()` populates the slot config fields on page load.
- Entry list is loaded via `GET /api/countdowns` on tab activation (lazy load, same pattern as wifi scan).
- Adding/editing an entry sends `POST /api/countdowns`.
- Delete button sends `DELETE /api/countdowns/{index}`.
- I18N keys added for both `en` and `pt` dictionaries.

### Todo List

1. Add tab button `<button class="tab" data-tab="countdowns">&#9203; <span data-i18n="tab.countdowns">Countdowns</span></button>` after the Quotes tab button.
2. Add tab panel `<div class="panel" id="tab-countdowns">` with:
   - Slot enable checkbox (`cfg-countdowns-en`), display interval input (`cfg-countdowns-display`), schedule fields (`cfg-countdowns-sched-start/end` + day mask toggles `cfg-countdowns-days`).
   - Save Countdown Config button calling `saveCountdownsConfig()`.
   - Entry list container `<div id="countdown-list">`.
   - Add-entry form: `<input id="cd-label">`, `<input type="datetime-local" id="cd-target">`, Save button calling `addCountdownEntry()`.
3. Add i18n keys to both `en` and `pt` dictionaries: `tab.countdowns`, `countdowns.slotConfig`, `countdowns.enabled`, `countdowns.displaySec`, `countdowns.schedule`, `countdowns.entries`, `countdowns.label`, `countdowns.target`, `countdowns.add`, `countdowns.noEntries`, `countdowns.delete`.
4. In `loadConfig()`, add population of the slot config fields (`countdowns_enabled`, `countdowns_display_ms`, `countdowns_sched_*`) from the `/api/config` response.
5. Add `saveCountdownsConfig()` JS function: reads slot config fields and POSTs to `/api/config`.
6. Add `loadCountdownEntries()` JS function: fetches `GET /api/countdowns` and renders the entry list.
7. Add `addCountdownEntry()` JS function: reads label + datetime-local input, converts to epoch (`Math.floor(new Date(val).getTime() / 1000)`), POSTs to `/api/countdowns`, then calls `loadCountdownEntries()`.
8. Add `deleteCountdownEntry(index)` JS function: sends `DELETE /api/countdowns/{index}`, then reloads the list.
9. Wire tab activation to call `loadCountdownEntries()` on first switch to the countdowns tab (lazy-load pattern).

### Relevant Context

- Tab button and panel pattern: any existing tab in [`web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:146) — all follow `data-tab="name"` + `id="tab-name"` convention; the tab-switching JS already handles any new tab automatically.
- Schedule fields helpers (`setScheduleFields`, `setDaysMask`) already exist in the JS — reuse directly.
- `loadConfig()` population pattern: lines 925–942 in [`web_page.cpp`](smart-matrix-clock-esp32/web_page.cpp:925) (weather + quotes blocks).
- `datetime-local` input value is a string like `"2025-01-01T00:00"` — convert via `new Date(val).getTime() / 1000` for epoch; display back via `new Date(epoch * 1000).toLocaleString()`.
- For Portuguese translation, key names to translate: tab label "Contagens Regressivas", "Adicionar", "Deletar", "Nenhuma entrada", etc.

---

## Constraints & Cross-Cutting Notes

- **`loop()` must never block** — `saveConfig()` writes NVS synchronously but is called only on user action (HTTP handler), never from `displayTick()` or `_slotRotationTick()`. The bell-message injection at zero-crossing happens inside `_buildCountdownString()` which is called from `displayTick()` — it only sets `messagePending` and writes `messageText[]`, which are O(1) operations; no NVS write happens there (NVS write deferred to `saveConfig()` called before returning from the HTTP handler, or immediately after zero-crossing inside display tick is acceptable since it is a one-time event).
- **Array bounds** — all slot arrays must be consistently sized to `[5]` across `globals.cpp`, `display.cpp` (`_slotLastShownMs`), `persistence.cpp`, and `web_routes.cpp`. A mismatch causes an out-of-bounds access and undefined behaviour.
- **NVS key length** — all new keys must be ≤ 15 characters. The existing test `test_persistence_language.cpp` checks this for any `NVS_KEY_*` constant defined in `config.h`.
- **`saveConfig()` from display tick** — calling `saveConfig()` from `_buildCountdownString()` (inside `displayTick()`) is the one exception to the "no I/O in display tick" rule, but it is justified: it fires at most once per entry (when a countdown reaches zero), not on every tick. Add a comment to that effect.
- **Sub-task order** — implement in order A → B → C → D → E → F. Each sub-task depends on the constants/structs defined by the previous ones.
