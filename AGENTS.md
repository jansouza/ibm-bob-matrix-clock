# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project

ESP32 firmware (Arduino/arduino-cli or PlatformIO) for a MAX7219 LED matrix clock. All 5 implementation phases are complete — see [`docs/implementation-plan.md`](docs/implementation-plan.md) for the roadmap and [`docs/project-spec.md`](docs/project-spec.md) for the full spec.

## Build & Flash Commands

Board FQBN: `esp32:esp32:esp32` (ESP32 Dev Module). **Source lives in the `smart-matrix-clock-esp32/` subdirectory.**

```bash
# Compile / verify (no device required — use this to check code before flashing)
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32

# Upload to device (replace /dev/ttyACM0 with the actual port)
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyACM0 \
  --upload-property upload.speed=115200 smart-matrix-clock-esp32

# Serial monitor (115200 baud)
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200

# PlatformIO equivalents (if pio is installed instead)
pio run            # compile
pio run -t upload  # upload
pio device monitor # serial monitor
```

Always run **both** validation steps after any code change before reporting completion:

```bash
# 1. Native unit tests (no device required) — must finish with ✓ ALL PASS
cd tests && make

# 2. Firmware compile check (no device required)
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
```

The test suite lives in [`tests/`](tests/) and runs on the host — no ESP32 needed. See [docs/testing.md](docs/testing.md) for the full reference.

### Required libraries (already installed)

| Library | Version |
|---|---|
| `MD_MAX72XX` | 3.5.1 |
| `MD_Parola` | 3.7.5 |
| `ESP Async WebServer` | 3.11.2 |
| `Async TCP` | 3.5.0 |
| `ArduinoJson` | 7.4.3 |

To install missing libraries: `arduino-cli lib install "MD_MAX72XX" "MD_Parola" "ESP Async WebServer" "AsyncTCP" "ArduinoJson"`

To install the ESP32 core (if not present): `arduino-cli core install esp32:esp32`

## Architecture: The Non-Obvious Parts

- **No blocking anywhere except `setup()`** — `delay()` is forbidden in `loop()`. Every timer uses `millis()`. Exception: `delay(200)` inside `_stationConnect()` in `wifi_manager.cpp` is explicitly allowed because it runs only during `setup()`.
- **HTTP handlers must only set state variables** — never call display or network I/O. The actual work happens in `loop()`.
- **`ESP.restart()` is never called directly** — always via `scheduleRestart(delayMs)` so the HTTP response reaches the client first.
- **`HTTPClient` is only called from `fetcherTick()`** — never inside HTTP handlers, because it is synchronous and would block the render loop.
- **Always use `http.getString()`, never `http.getStream()`** — chunked/compressed responses are unreliable with `getStream()` on ESP32, producing spurious parse errors on valid payloads.
- **All display strings must be UTF-8 → Latin-1 converted** before passing to `MD_Parola` / `MD_MAX72XX`. `messageText[]` is stored in Latin-1, not UTF-8.
- **Message pipeline order matters**: `utf8ToLatin1()` first, then `expandIconTags()` — icon tags contain ASCII-range brackets that survive the encoding step.
- **JSON always built/parsed with `ArduinoJson`** — never manual string concatenation.
- **Message slot is one-shot**: `messagePending` is set by the HTTP handler and cleared by `displayTick()` after one scroll completes. Only the most recent message is kept.
- **Slot 0 (clock) is the permanent base** — it has no interval (`slotIntervalMs[0] = 0`); other slots return to it after their scroll ends.
- **`applyTimezone()` must run after `ntpBegin()`** — `configTime()` resets TZ to UTC internally. Also called inside `ntpTick()` during periodic re-sync (every `NTP_RESYNC_MS`) for the same reason.
- **Two separate language settings**: `cfgLanguage` controls the on-device clock/date locale (weekday/month names); `cfgUiLanguage` controls the web panel's own UI language. They are stored independently in NVS.
- **No authentication on any endpoint** — an API-key mechanism was implemented and removed because the web panel's JS never sent the header, locking the panel out of its own write actions.

## Module Layout

| File | Responsibility |
|---|---|
| `config.h` | All pin definitions, buffer sizes, default values, NVS keys |
| `globals.h/cpp` | Shared state between HTTP layer and display layer (extern vars) |
| `display.h/cpp` | MD_Parola rendering, blink, scroll, slot rotation manager |
| `wifi_manager.h/cpp` | WiFi connect, setup AP, deferred restart |
| `ntp.h/cpp` | `configTime()` wrapper, sync polling, periodic re-sync |
| `text_encoding.h/cpp` | UTF-8 ↔ Latin-1 conversion; `expandIconTags()` for `[heart]`/`[bell]`/`[warn]` etc. |
| `locale_data.h/cpp` | Day/month names by language, IANA timezone → POSIX TZ table, WMO weathercode strings |
| `persistence.h/cpp` | NVS load/save via `Preferences`, `applyTimezone()`, `factoryReset()` |
| `date_font.h` | Custom bitmap font for date display |
| `web_page.h/cpp` | Single self-contained HTML/CSS/JS string literal |
| `web_routes.h/cpp` | `ESPAsyncWebServer` route registration |
| `data_fetcher.h/cpp` | Open-Meteo + Yahoo Finance HTTP fetch, cache structs |

## Key Globals (globals.h)

- `ntpSynced` — set true by `ntpTick()`, read by `displayTick()` to switch `--:--` → `HH:MM`
- `messagePending` / `messageText[]` — set by HTTP handler, consumed (and cleared) by `displayTick()`
- `messageBrightness` / `messageScrollSpeedMs` — per-message overrides; `-1` means use configured global value
- `slotEnabled[4]` — `{clock, message, weather, quotes}` — disabled slots are silently skipped
- `slotIntervalMs[4]` — clock slot is `0` (permanent base); message slot is `0` (one-shot); default values in `globals.cpp` are `{0, 0, 60000, 120000}` regardless of `config.h` display defaults
- `slotScheduleStartMin[slot]` / `slotScheduleEndMin[slot]` — daily window in minutes-of-day (0–1439); **`start == end` means "always active"** — only slots 2 and 3 (weather/quotes) use this
- `slotScheduleDaysMask[slot]` — bitmask of enabled weekdays; bit N corresponds to `tm_wday` (0=Sunday, 6=Saturday); `0x7F` = all days
- `messageHistory[]` — ring buffer of last 20 messages; managed via `messageHistoryHead` (oldest index) and `messageHistoryCount`; exposed by `GET /api/messages/history`

## Hardware

- 4× MAX7219 FC16 8×8 LED modules chained via VSPI: CLK=18, DATA/MOSI=23, CS=5
- Display type constant: `MD_MAX72XX::FC16_HW` — must match physical hardware type or columns will be scrambled
- **FC16_HW column direction**: raw column 0 is the **rightmost** physical pixel, raw column 31 is the leftmost — critical for any direct pixel manipulation via `getGraphicObject()`; `_writeSmallDigit()` compensates with `setColumn(31 - visualCol - i, ...)`
- BOOT button (GPIO 0, active-LOW) held at power-on triggers factory reset

## NTP / Timezone

- `configTime(0, 0, server)` always sets UTC offsets to 0 — timezone is handled entirely via POSIX TZ string through `setenv("TZ", ...)` + `tzset()`
- Clock validity check: `tm_year + 1900 >= 2020` (year < 2020 means epoch, i.e., not synced)
- Periodic re-sync calls `configTime()` again; `ntpSynced` stays `true` during re-sync (clock keeps last value); `applyTimezone()` must be re-applied immediately after or the display reverts to UTC

## Data Sources

- **Weather**: Open-Meteo — free, no API key; uses `current=temperature_2m,weathercode` + `daily=temperature_2m_max,temperature_2m_min` in a single request
- **Quotes**: Yahoo Finance `v8/finance/chart/{symbol}` (per-symbol, not batch) — **not** `v7/finance/quote` (that endpoint 401s without a session cookie). A fake desktop User-Agent with `http.setUserAgent(...)` is required — Yahoo rejects the default ESP32 UA.

## Failure / Cache Policy

- If a slot's data fetch fails and there is no cache → slot is silently skipped that rotation cycle
- If cache exists but fetch failed → display with `*` prefix (e.g., `*22°C Nublado`)

## Clock Mode (HH:MM:SS)

- `cfgClockMode` — `CLOCK_MODE_HHMM` (0, default) or `CLOCK_MODE_HHMMSS` (1); persisted in NVS key `"clock_mode"`
- In HHMMSS mode: `HH:MM` is rendered with `PA_LEFT` via `_display.print()`; seconds are overlaid via direct `setColumn()` calls (does **not** clear the display). The seconds position is computed dynamically by `_ssLayout(hmWidthPx)` — **not** a fixed column — so `HH:MM` width is measured first with `getTextWidth()`.
- Changing `cfgClockMode` at runtime requires calling `displayForceRedraw()` (sets `_lastTimeStr[0] = '\0'` and resets alignment to `PA_CENTER`) — otherwise the display keeps stale layout state.
- `/api/status` returns `time_str` as `HH:MM:SS` when `cfgClockMode == CLOCK_MODE_HHMMSS`, used by the web panel live preview (polled every 1 second).

## Adding a New UI Language

1. Add the code string to `_uiLanguages[]` in `persistence.cpp` — this is the **only** validation source.
2. Add a matching I18N dictionary entry in `web_page.cpp`.
No other branching logic is needed.

## Adding a New Slot

1. Add an index constant in `config.h`.
2. Initialize `slotEnabled[]` and `slotIntervalMs[]` entry in `globals.cpp`.
3. Add cache struct in `globals.h/cpp`.
4. Add fetch logic in `data_fetcher.cpp` called from `fetcherTick()`.
5. Add rendering branch in `slotRotationTick()` in `display.cpp`.
6. Expose enable/interval config fields through `POST /api/config` in `web_routes.cpp`.
7. Persist the new fields in `persistence.cpp`.

## Naming Conventions

- Module-private state variables: `static` in `.cpp`, prefixed with `_` (e.g., `_lastBlink`, `_colonVisible`).
- Public API functions: `camelCase` with module prefix (e.g., `displayTick()`, `ntpBegin()`, `wifiConnect()`).
- Constants in `config.h`: `SCREAMING_SNAKE_CASE`.
- `extern` globals in `globals.h`: no prefix, plain `camelCase` (e.g., `ntpSynced`, `messagePending`).

## Include Order (per file)

1. Own header (e.g., `#include "display.h"`)
2. Project headers (e.g., `#include "config.h"`, `#include "globals.h"`)
3. Arduino/ESP32 library headers (e.g., `#include <MD_Parola.h>`, `#include <Arduino.h>`)
4. C standard headers (e.g., `#include <time.h>`)

## Display Write Optimisation

`displayTick()` compares the new time string against `_lastTimeStr` and only calls `_display.print()` when the content actually changed — preserve this guard when modifying the render path. The MAX7219 write is the expensive/blocking part of each tick.

## FC16_HW Column Direction (direct pixel work only)

Raw column 0 is the **rightmost** physical pixel; raw column 31 is the leftmost. The `_startDateDisplay()` implementation in `display.cpp` has a detailed comment explaining how this affects repositioning and day-of-week decoration loops — read it before doing any `getGraphicObject()` pixel manipulation.

## Yahoo Finance API

Use `v8/finance/chart/{symbol}` — **not** `v7/finance/quote` (that endpoint 401s without a session cookie). Set a fake desktop User-Agent with `http.setUserAgent(...)` — Yahoo rejects the default ESP32 UA.

## Test Suite

The test suite is a native host build — no ESP32, no simulator, no special toolchain. It compiles and runs with plain `g++`.

### Running the tests

```bash
cd tests
make          # build + run (shows per-suite results and final ✓ / ✗ summary)
make build    # build only
make clean    # remove build artefacts
```

The runner exits with code `0` when all tests pass and `1` when any test fails. CI should check the exit code.

### Test file map

| File | Module under test | What is covered |
|---|---|---|
| [`tests/test_text_encoding.cpp`](tests/test_text_encoding.cpp) | `text_encoding.cpp` | `utf8ToLatin1`, `latin1ToUtf8`, `expandIconTags` (all 11 icons, unknown tags, buffer limits, null guards), `formatQuotePrice` |
| [`tests/test_locale_data.cpp`](tests/test_locale_data.cpp) | `locale_data.cpp` | Day/month names EN + PT, `ianaToPostfix` (known zones + edge cases), `tzTableEntry` consistency, all WMO codes EN + PT |
| [`tests/test_slot_schedule.cpp`](tests/test_slot_schedule.cpp) | `display.cpp` (schedule algorithm) | Same-day window, always-active sentinel (`start == end`), midnight-crossing window, day-mask gating, night-brightness window |
| [`tests/test_data_fetcher_split.cpp`](tests/test_data_fetcher_split.cpp) | `data_fetcher.cpp` (ticker split) | Comma splitting, whitespace trimming, empty tokens, max-symbol cap, buffer safety |
| [`tests/test_persistence_language.cpp`](tests/test_persistence_language.cpp) | `persistence.cpp` / `config.h` | `isUiLanguageValid`, `formatQuotePrice` separators, NVS key length ≤ 15, config constant sanity |

### What modules are NOT covered by host tests

These modules have hard dependencies on Arduino/ESP32 runtime APIs and cannot be tested without hardware:

| Module | Reason |
|---|---|
| `display.cpp` | Requires `MD_Parola` / `MD_MAX72XX` hardware objects |
| `wifi_manager.cpp` | Requires `WiFi.*` ESP32 stack |
| `ntp.cpp` | Requires `configTime()` / `getLocalTime()` ESP32 APIs |
| `persistence.cpp` | Requires `Preferences` (NVS flash) |
| `web_routes.cpp` | Requires `ESPAsyncWebServer` |
| `data_fetcher.cpp` | Requires `HTTPClient` and a live network |

For these modules, validation is done on-device after flashing.

### Adding tests for new logic

When you add or modify a **pure-logic function** (no Arduino/ESP32 calls) in any module:

1. **Decide the file.** If the function lives in `text_encoding.cpp` or `locale_data.cpp`, add tests directly to the matching test file. For logic extracted from other modules (like the schedule algorithm in `display.cpp`), add a new `test_<module>.cpp`.

2. **Add a new test file** (if needed):
   - Add `test_<name>.cpp` in `tests/`.
   - Register it in the `TEST_SRCS` list in [`tests/Makefile`](tests/Makefile).
   - Include `framework.h` and declare `SUITE`/`TEST` blocks — no `main()` needed.

3. **Write SUITE + TEST blocks.** Group related assertions into named suites:
   ```cpp
   #include "framework.h"
   #include "my_module.h"    // or replicate the algorithm inline

   SUITE("myFunction — normal inputs") {
       TEST("zero input returns zero") {
           ASSERT_EQ(myFunction(0), 0);
       }
       TEST("negative input is clamped") {
           ASSERT_TRUE(myFunction(-1) >= 0);
       }
   }
   ```

4. **For private (static) functions:** replicate the algorithm in the test file with a comment `// Algorithm replica from <file>.cpp / <function>()`. Add a note that the replica must be kept in sync if the original changes.

5. **Run `cd tests && make`** — must finish `✓ ALL PASS` with no warnings before committing.

6. **Also run the firmware compile** — test files must never change firmware source:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
   ```

### Framework macros reference

| Macro | Usage |
|---|---|
| `SUITE("name") { ... }` | Declares and registers a test suite |
| `TEST("desc") { ... }` | Named test case inside a suite |
| `ASSERT_EQ(a, b)` | Equality — prints values on failure |
| `ASSERT_NE(a, b)` | Inequality |
| `ASSERT_TRUE(expr)` | Truthy |
| `ASSERT_FALSE(expr)` | Falsy |
| `ASSERT_STREQ(s1, s2)` | `strcmp(s1, s2) == 0` |
| `ASSERT_STRNE(s1, s2)` | `strcmp(s1, s2) != 0` |
| `ASSERT_NEAR(a, b, eps)` | `|a - b| <= eps` (floats) |

