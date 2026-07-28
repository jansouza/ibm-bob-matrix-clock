# Code Fixes — Smart Matrix Clock

Issues found during code review that do not require a new feature, only a targeted code change. Each item is self-contained and can be fixed and reviewed independently.

Reference: [`docs/enhancements-plan.md`](enhancements-plan.md) | [`docs/project-spec.md`](project-spec.md)

---

## Summary

| # | Severity | File | GitHub Issue | Status |
|---|---|---|---|---|
| C1 | ⚪ Not an issue | `persistence.cpp · applyTimezone()` | [#1](https://github.com/jansouza/ibm-bob-matrix-clock/issues/1) | `[x] invalid — closed` |
| C2 | 🟡 Bug Risk | `display.cpp · _slotInSchedule()` | [#2](https://github.com/jansouza/ibm-bob-matrix-clock/issues/2) | `[ ] pending` |
| C3 | 🟢 Cleanup | `persistence.cpp · loadConfig()` | [#3](https://github.com/jansouza/ibm-bob-matrix-clock/issues/3) | `[ ] pending` |
| C4 | 🟡 Robustness | `data_fetcher.cpp · _fetchOneQuote()` + `_fetchWeather()` | [#4](https://github.com/jansouza/ibm-bob-matrix-clock/issues/4) | `[ ] pending` |
| C5 | 🟡 Robustness | `smart-matrix-clock-esp32.ino · setup()` | [#5](https://github.com/jansouza/ibm-bob-matrix-clock/issues/5) | `[ ] pending` |
| C6 | 🟢 Cleanup | `config.h` | [#6](https://github.com/jansouza/ibm-bob-matrix-clock/issues/6) | `[ ] pending` |
| C7 | 🟢 Cleanup | `data_fetcher.cpp · _fetchWeather()` | [#7](https://github.com/jansouza/ibm-bob-matrix-clock/issues/7) | `[ ] pending` |
| C8 | 🟢 Cleanup | `config.h · BOOT_HOLD_MS` | — | `[ ] pending` |
| C9 | 🟢 Cleanup | `config.h · NVS_KEY_MESSAGE_MODE/DUR` | — | `[ ] pending` |

Verified against the current tree on 2026-07-27. C1 was re-checked and found already resolved (see below); C2–C7 were re-confirmed present; C4 and C8 were found to be broader than originally scoped; C8 and C9 are new findings from this pass.

---

## C1 — `applyTimezone()` — ~~undefined behaviour on unknown timezone~~ NOT AN ISSUE [\#1](https://github.com/jansouza/ibm-bob-matrix-clock/issues/1)

**Severity:** ⚪ Not an issue (re-verified 2026-07-27)
**File:** `persistence.cpp · applyTimezone()`

### Original claim

When `ianaToPostfix(cfgTimezone)` returns `nullptr`, the code substitutes `NTP_TIMEZONE_DEFAULT` (`"UTC"`) and calls `ianaToPostfix("UTC")` again. The claim was that `"UTC"` is not in the IANA lookup table (which allegedly uses `"Etc/UTC"`), so the second lookup would also return `nullptr` and fall through to `setenv("TZ", nullptr, 1)` — undefined behaviour on POSIX systems.

### Why this does not hold today

`locale_data.cpp`'s `_tzTable` has a literal entry:

```cpp
{ "UTC", "UTC0" },
```

`NTP_TIMEZONE_DEFAULT` is `"UTC"` (`config.h`), so `ianaToPostfix(NTP_TIMEZONE_DEFAULT)` always resolves to `"UTC0"` on the fallback path — it can never return `nullptr`, and `setenv()` never receives a null pointer. The double-lookup in `applyTimezone()` is safe as written:

```cpp
void applyTimezone() {
    const char* posix = ianaToPostfix(cfgTimezone);
    if (posix == nullptr) {
        // Fall back to Sao Paulo POSIX if unknown
        posix = ianaToPostfix(NTP_TIMEZONE_DEFAULT);
    }
    setenv("TZ", posix, 1);
    tzset();
}
```

### Minor nit (not worth its own issue)

The comment `// Fall back to Sao Paulo POSIX if unknown` is stale — the fallback is UTC, not São Paulo. Harmless; fix opportunistically if touching this function for another reason.

### Disposition

Close #1 as invalid — either the underlying table already had the `"UTC"` entry when the issue was filed and the report was mistaken, or it was fixed since without updating this doc. No code change needed.

---

## C2 — `_slotInSchedule()` — incorrect weekday before NTP sync [\#2](https://github.com/jansouza/ibm-bob-matrix-clock/issues/2)

**Severity:** 🟡 Bug Risk
**File:** `display.cpp · _slotInSchedule()`
**Status:** Confirmed present, `display.cpp` lines 456–459.

### Problem

`_slotInSchedule()` calls `time(nullptr)` which returns the Unix epoch (`1970-01-01 Thursday`) before NTP syncs. Depending on the day-mask configured for a slot, the slot could be incorrectly allowed or blocked during the ~30 s window before the first NTP sync completes.

```cpp
static bool _slotInSchedule(uint8_t slot) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);

    if (!(slotScheduleDaysMask[slot] & (1 << t.tm_wday))) return false;
    // ... unchanged
```

### Fix

Early-return `false` when the clock is not yet synced:

```cpp
static bool _slotInSchedule(uint8_t slot) {
    if (!ntpSynced) return false;   // epoch time gives wrong tm_wday
    // ... rest unchanged
```

### Impact

Minor visual glitch (wrong slot shown briefly at boot) on devices with specific day-mask configurations. Zero performance cost to fix. Note: `tests/test_slot_schedule.cpp` tests the pure scheduling algorithm but does not cover this `ntpSynced` gate, since the gate lives in `display.cpp` around the call, not in the algorithm itself.

---

## C3 — `loadConfig()` — `slotEnabled[2]` and `slotEnabled[3]` written twice [\#3](https://github.com/jansouza/ibm-bob-matrix-clock/issues/3)

**Severity:** 🟢 Cleanup
**File:** `persistence.cpp · loadConfig()`
**Status:** Confirmed present — and broader than originally scoped: **both** slot 2 (weather) and slot 3 (quotes) are double-written, not just slot 2.

### Problem

`slotEnabled[2]` is assigned twice in `loadConfig()`:

1. `persistence.cpp:65`: `slotEnabled[2] = _prefs.getBool(NVS_KEY_SLOT2_EN, false)` — generic slot array
2. `persistence.cpp:84`: `slotEnabled[2] = _prefs.getBool(NVS_KEY_WEATHER_EN, false)` — weather-specific key

The same pattern repeats for `slotEnabled[3]`:

1. `persistence.cpp:66`: `slotEnabled[3] = _prefs.getBool(NVS_KEY_SLOT3_EN, false)`
2. `persistence.cpp:92`: `slotEnabled[3] = _prefs.getBool(NVS_KEY_QUOTES_EN, false)`

The second write always wins in each pair (correct behaviour today), but `saveConfig()` persists both keys in both pairs, so `NVS_KEY_SLOT2_EN`/`NVS_KEY_WEATHER_EN` and `NVS_KEY_SLOT3_EN`/`NVS_KEY_QUOTES_EN` are always in sync and one key in each pair is redundant. If a future partial-save path writes only one key, they could silently diverge.

### Fix

Remove the generic `NVS_KEY_SLOT2_EN` / `NVS_KEY_SLOT3_EN` read/write for indices 2 and 3 from both `loadConfig()` and `saveConfig()`, keeping only the weather/quotes-specific keys (`NVS_KEY_WEATHER_EN`, `NVS_KEY_QUOTES_EN`). Slot 0 (clock) and slot 1 (message) have no weather/quotes counterpart, so `NVS_KEY_SLOT0_EN` and `NVS_KEY_SLOT1_EN` are untouched and should be kept as-is.

*(The original write-up of this fix incorrectly listed `NVS_KEY_SLOT0_EN` among the keys to remove — that was a typo; slot 0 was never meant to be touched.)*

---

## C4 — no guard against empty `getString()` response, in both fetchers [\#4](https://github.com/jansouza/ibm-bob-matrix-clock/issues/4)

**Severity:** 🟡 Robustness
**File:** `data_fetcher.cpp · _fetchOneQuote()` **and** `_fetchWeather()`
**Status:** Confirmed present in `_fetchOneQuote()`; also found in `_fetchWeather()`, which was not covered by the original issue.

### Problem

`http.getString()` returns an empty `String` if the heap allocation fails (fragmented heap after hours of operation). The subsequent `deserializeJson(doc, body)` call then returns `EmptyInput`, which is logged as a generic parse error — indistinguishable from a malformed server response. No retries are attempted, and the failure is not categorised.

This affects both HTTP fetch paths, since they share the same pattern:

- `_fetchOneQuote()` — `data_fetcher.cpp:177-181`
- `_fetchWeather()` — `data_fetcher.cpp:83-87`

### Fix

Check `body.length()` before parsing and log a distinct warning, in **both** functions:

```cpp
String body = http.getString();
http.end();
if (body.length() == 0) {
    Serial.printf("[Quotes] %s empty body (heap alloc failed?)\n", symbol);
    return false;
}
```

```cpp
String body = http.getString();
http.end();
if (body.length() == 0) {
    Serial.println("[Weather] empty body (heap alloc failed?)");
    if (weatherCache.valid) weatherCache.stale = true;
    return;
}
```

### Impact

Makes heap exhaustion failures observable in the serial monitor for both weather and quotes. Does not prevent the failure itself, but distinguishes it from a real parse error.

---

## C5 — Factory reset BOOT button — no debounce, and no hold-duration check [\#5](https://github.com/jansouza/ibm-bob-matrix-clock/issues/5)

**Severity:** 🟡 Robustness
**File:** `smart-matrix-clock-esp32.ino · setup()`
**Status:** Confirmed present, `smart-matrix-clock-esp32.ino:34-38`. Broader than originally scoped — see below.

### Problem

The BOOT button check fires immediately if GPIO 0 is low at power-on with no debounce and no hold-confirmation delay. Some USB-Serial adapters assert DTR (which is connected to GPIO 0 / BOOT on many ESP32 dev boards) briefly during the USB enumeration phase, which can trigger a factory reset on every USB plug-in.

```cpp
pinMode(PIN_BOOT, INPUT_PULLUP);
if (digitalRead(PIN_BOOT) == LOW) {
    Serial.println("[Boot] BOOT button held — factory reset");
    factoryReset();
    // Continue to AP mode with cleared config
}
```

**Additional finding:** `config.h` already defines `BOOT_HOLD_MS 3000` — "hold duration to trigger factory reset (ms)" — but this constant is never referenced anywhere in the codebase. This suggests a 3-second hold-to-confirm was the original intent (matching how BOOT-button factory resets typically work on other devices) and was never wired up, not just a missing 50 ms debounce. See also [C8](#c8--dead-config-constant-boot_hold_ms).

### Fix

Use the existing `BOOT_HOLD_MS` constant to require the button to stay held for the full duration, polling with a short debounce interval, instead of a bare 50 ms check:

```cpp
if (digitalRead(PIN_BOOT) == LOW) {
    uint32_t heldSince = millis();
    bool stillHeld = true;
    while (millis() - heldSince < BOOT_HOLD_MS) {
        delay(50);
        if (digitalRead(PIN_BOOT) != LOW) { stillHeld = false; break; }
    }
    if (stillHeld) {
        Serial.println("[Boot] BOOT button held — factory reset");
        factoryReset();
    }
}
```

The blocking wait is acceptable here because it runs only once in `setup()`, which already contains longer blocking calls (WiFi connect timeout, NTP wait).

### Impact

Eliminates accidental factory resets triggered by USB-Serial DTR pulses at power-on (which are brief, well under 3 s), and makes `BOOT_HOLD_MS` a live constant instead of dead configuration.

---

## C6 — Slot index literals `2` and `3` hardcoded throughout codebase [\#6](https://github.com/jansouza/ibm-bob-matrix-clock/issues/6)

**Severity:** 🟢 Cleanup
**File:** `config.h` (and all callers)
**Status:** Confirmed present — no `SLOT_WEATHER`/`SLOT_QUOTES` constants exist today. Literal `2`/`3` slot references confirmed in `display.cpp`, `data_fetcher.cpp`, `web_routes.cpp`, `persistence.cpp`.

### Problem

The integer literals `2` (weather) and `3` (quotes) are used directly across `display.cpp`, `data_fetcher.cpp`, `web_routes.cpp`, and `persistence.cpp`. This makes the code harder to read and makes slot renumbering or reordering a multi-file error-prone search-and-replace.

### Fix

Add symbolic constants to `config.h`:

```cpp
#define SLOT_CLOCK    0
#define SLOT_MESSAGE  1
#define SLOT_WEATHER  2
#define SLOT_QUOTES   3
```

Then replace all bare `2` and `3` slot references in the codebase with `SLOT_WEATHER` and `SLOT_QUOTES`.

### Impact

Zero runtime change; purely improves readability and makes future slot additions safer.

---

## C7 — `_fetchWeather()` — duplicated URL builder for Celsius/Fahrenheit [\#7](https://github.com/jansouza/ibm-bob-matrix-clock/issues/7)

**Severity:** 🟢 Cleanup
**File:** `data_fetcher.cpp · _fetchWeather()`
**Status:** Confirmed present, `data_fetcher.cpp:48-65`.

### Problem

The Celsius and Fahrenheit URL variants are built by two nearly identical `snprintf` blocks that share every query parameter except the optional `&temperature_unit=fahrenheit` suffix. Any future change to the URL (e.g. adding a new field, changing the API version) must be applied twice and kept in sync manually.

### Fix

Use a single `snprintf` with a conditional unit suffix:

```cpp
snprintf(url, sizeof(url),
    "http://api.open-meteo.com/v1/forecast"
    "?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,weathercode"
    "&daily=temperature_2m_max,temperature_2m_min"
    "%s"
    "&forecast_days=1&timezone=auto",
    cfgWeatherLat, cfgWeatherLon,
    isFahrenheit ? "&temperature_unit=fahrenheit" : "");
```

### Impact

Zero runtime change; eliminates duplication and the associated drift risk.

---

## C8 — Dead config constant `BOOT_HOLD_MS` [new, found 2026-07-27]

**Severity:** 🟢 Cleanup
**File:** `config.h`

### Problem

`config.h:18` defines `#define BOOT_HOLD_MS 3000  // hold duration to trigger factory reset (ms)`, but no `.cpp`/`.ino` file references it. The factory-reset check in `setup()` doesn't implement a hold duration at all (see [C5](#c5--factory-reset-boot-button--no-debounce-and-no-hold-duration-check-5)).

### Fix

Resolved as part of the C5 fix above (wiring `BOOT_HOLD_MS` into the hold-to-confirm loop). If C5 is deferred, this should at minimum get a `// TODO` or be removed to avoid implying behaviour that doesn't exist.

### Impact

No runtime effect either way; purely a documentation/intent mismatch until C5 is fixed.

---

## C9 — Dead NVS key constants `NVS_KEY_MESSAGE_MODE` / `NVS_KEY_MESSAGE_DUR` [new, found 2026-07-27]

**Severity:** 🟢 Cleanup
**File:** `config.h`

### Problem

`config.h:112-113` define `NVS_KEY_MESSAGE_MODE` (`"msg_mode"`) and `NVS_KEY_MESSAGE_DUR` (`"msg_dur"`), but neither is referenced in `persistence.cpp`'s `loadConfig()`/`saveConfig()`/`factoryReset()`, nor anywhere else. `messageMode` and `messageDurationMs` are set purely per-request by the `POST /api/message` handler (`web_routes.cpp:444`) and are intentionally transient — Slot 1 is one-shot per `CLAUDE.md`, so persisting them would not make sense. The two `#define`s appear to be leftover from an earlier design.

### Fix

Delete both `#define`s from `config.h` unless there's a concrete plan to persist a default message mode/duration.

### Impact

Zero runtime effect; removes misleading dead code that suggests message settings are persisted when they aren't.

---

## Recommended fix order

```
C2  (_slotInSchedule epoch)   ← minor bug; 1-line fix
C5  (BOOT hold + debounce)    ← prevents accidental resets; also resolves C8
C4  (getString empty guard)   ← improves diagnosability in both fetchers
C6  (slot index constants)    ← prerequisite for any new slot work
C3  (double NVS write)        ← cleanup; safe to do alongside C6
C7  (URL builder dedup)       ← cosmetic cleanup; lowest priority
C9  (dead NVS key defines)    ← trivial deletion, no dependencies
C1  ← closed, no action needed
```
