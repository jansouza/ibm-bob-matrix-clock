# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project

ESP32 firmware (Arduino/arduino-cli or PlatformIO) for a MAX7219 LED matrix clock. Source lives in `smart-matrix-clock-esp32/`. Board FQBN: `esp32:esp32:esp32`.

## Validation (run both after every code change)

```bash
# 1. Host unit tests — must finish ✓ ALL PASS
cd tests && make

# 2. Firmware compile — no device required
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
```

To run only specific test files: the test suite has no per-file runner; `cd tests && make` runs all of them together. To add or isolate test focus, add a new `test_<name>.cpp` and register it in `tests/Makefile` `TEST_SRCS`.

## Documentation Rule

After every implementation, update **all four** of these before reporting completion:

1. `AGENTS.md` — architecture notes, gotchas
2. `README.md` — REST API table, features list
3. `docs/api-rest.md` — new/modified endpoint docs
4. `docs/enhancements-plan.md` — mark sub-task `[x] done`, move to "Implemented" table

## Hard Rules (violating these breaks things)

- **No `delay()` in `loop()`** — use `millis()`-based timers. Exception: `delay(200)` in `_stationConnect()` (`wifi_manager.cpp`) runs only during `setup()`.
- **No `ESP.restart()` directly** — always use `scheduleRestart()` from `wifi_manager.cpp` so the HTTP response reaches the client first.
- **HTTP handlers only set state variables** — no display or network I/O. Side-effects happen in the next `loop()` iteration.
- **`HTTPClient` only inside `fetcherTick()`** — never inside HTTP handlers.
- **Use `http.getString()`, not `http.getStream()`** — chunked/compressed ESP32 responses are unreliable with `getStream()`, causing spurious JSON parse errors.
- **UTF-8 → Latin-1 then `expandIconTags()`** — order matters. `utf8ToLatin1()` first, then `expandIconTags()`. `messageText[]` is stored Latin-1.
- **Always use `ArduinoJson`** for JSON — never string concatenation.
- **`applyTimezone()` must run after `ntpBegin()`** in `setup()`, and again inside `ntpTick()` after each re-sync, because `configTime()` resets TZ to UTC internally every call.

## Naming / Language Globals — Common Pitfall

The codebase has **two independent language settings** — confusing names, distinct purposes:

- `cfgLocale` (`globals.h`, NVS key `"locale"`) — on-device content locale: date weekday/month names, number formatting, quote search language. Validated against `locale_data.cpp`.
- `cfgUiLanguage` (`globals.h`, NVS key `"ui_lang"`) — web panel UI language. Validated against `_uiLanguages[]` in `persistence.cpp` only.

There is no `cfgLanguage` variable — older docs that say so are wrong.

## Module Layout

| File | Responsibility |
|---|---|
| `config.h` | All pin definitions, buffer sizes, defaults, NVS keys (≤15 chars each) |
| `globals.h/cpp` | Shared state between HTTP layer and display layer (`extern` vars) |
| `display.h/cpp` | MD_Parola rendering, blink, scroll, slot rotation |
| `wifi_manager.h/cpp` | WiFi connect, setup AP, `scheduleRestart()` |
| `ntp.h/cpp` | `configTime()` wrapper, sync polling, re-sync |
| `text_encoding.h/cpp` | UTF-8 ↔ Latin-1 conversion; `expandIconTags()` |
| `locale_data.h/cpp` | Day/month names, IANA→POSIX TZ table, WMO weather codes |
| `persistence.h/cpp` | NVS load/save, `applyTimezone()`, `factoryReset()`, `isUiLanguageValid()` |
| `web_page.h/cpp` | Single monolithic `~62 KB` raw-string literal `WEB_PAGE_HTML` — entire UI |
| `web_routes.h/cpp` | `ESPAsyncWebServer` route registration |
| `data_fetcher.h/cpp` | Open-Meteo + Yahoo Finance HTTP fetch, cache structs |

**Counterintuitive locations**: `applyTimezone()` is in `persistence.cpp` not `ntp.cpp`; `expandIconTags()` is in `text_encoding.cpp` not `display.cpp`; `scheduleRestart()` is in `wifi_manager.cpp` but used for all restarts (not just WiFi).

## Key Globals (globals.h)

- `ntpSynced` — set by `ntpTick()`, read by `displayTick()` to switch `--:--` → `HH:MM`
- `messagePending` / `messageText[]` — set by HTTP handler, consumed (cleared) by `displayTick()`
- `messageMode` — `MESSAGE_MODE_SCROLL` (0), `MESSAGE_MODE_BLINK` (1), `MESSAGE_MODE_STATIC` (2), `MESSAGE_MODE_BLINK_SCROLL` (3)
- `messageBrightness` / `messageScrollSpeedMs` — per-message overrides; `-1` means use configured global
- `slotEnabled[4]` — `{clock, message, weather, quotes}` — disabled slots are silently skipped
- `slotIntervalMs[4]` — defaults in `globals.cpp` are `{0, 0, 60000, 120000}`; clock=0 (permanent base), message=0 (one-shot)
- `slotScheduleStartMin[slot]` / `slotScheduleEndMin[slot]` — minutes-of-day (0–1439); **`start == end` = "always active"** (not invalid); only slots 2/3 use this
- `slotScheduleDaysMask[slot]` — bit N = `tm_wday` (0=Sunday … 6=Saturday); `0x7F` = all days; only slots 2/3 use this
- `messageHistory[]` — ring buffer, 20 entries; `messageHistoryHead` = oldest index; iterate oldest-to-newest: `idx = (messageHistoryHead + i) % MESSAGE_HISTORY_SIZE`

## Hardware

- 4× MAX7219 FC16 8×8 modules via VSPI: CLK=18, DATA/MOSI=23, CS=5
- **`MD_MAX72XX::FC16_HW` column direction**: raw column 0 = rightmost pixel, raw column 31 = leftmost. `_writeSmallDigit()` compensates with `setColumn(31 - visualCol - i, ...)`. Read `_startDateDisplay()` in `display.cpp` before any direct pixel work.
- BOOT button (GPIO 0, active-LOW) held at power-on triggers factory reset

## Display Render Pipeline

- `displayTick()` compares against `_lastTimeStr` before calling `_display.print()` — preserve this guard; the MAX7219 write is the expensive part.
- `_display.print()` **clears the entire display**. In `CLOCK_MODE_HHMMSS`, `HH:MM` is rendered with `PA_LEFT`, then seconds are overlaid via `setColumn()` *after* — never before.
- Seconds start column is **dynamic**: `_ssLayout(hmWidthPx)` measures `HH:MM` pixel width via `getTextWidth()` first. Never hardcode a column.
- Changing `cfgClockMode` at runtime requires `displayForceRedraw()` to reset `_lastTimeStr` and alignment state.

## Data Sources

- **Weather**: Open-Meteo (no API key); `current=temperature_2m,weathercode` + `daily=temperature_2m_max,temperature_2m_min` in one request.
- **Quotes**: Yahoo Finance `v8/finance/chart/{symbol}` per-symbol — **not** `v7/finance/quote` (401s without session cookie). Requires a fake desktop User-Agent via `http.setUserAgent(...)`.
- Cache policy: fetch failed + no cache → slot silently skipped; fetch failed + cache exists → display with `*` prefix.

## Naming Conventions

- Module-private state: `static` in `.cpp`, prefixed `_` (e.g., `_lastBlink`)
- Public API functions: `camelCase` with module prefix (e.g., `displayTick()`, `ntpBegin()`)
- Constants in `config.h`: `SCREAMING_SNAKE_CASE`
- `extern` globals in `globals.h`: no prefix, plain `camelCase` (e.g., `ntpSynced`)
- NVS keys: defined in `config.h`, must be ≤15 characters

## Include Order (per file)

1. Own header
2. Project headers (`config.h`, `globals.h`, etc.)
3. Arduino/ESP32 library headers (`<MD_Parola.h>`, `<Arduino.h>`, etc.)
4. C standard headers (`<time.h>`, `<string.h>`, etc.)

## Test Suite

Host build — plain `g++`, no ESP32 required. Lives in `tests/`. Framework in `tests/framework.h` (custom `SUITE`/`TEST`/`ASSERT_*` macros).

- New test file: add `test_<name>.cpp` to `tests/`, register in `TEST_SRCS` in `tests/Makefile`.
- For private (`static`) functions: replicate the algorithm inline in the test file with comment `// Algorithm replica from <file>.cpp / <function>()`.
- Modules with Arduino/ESP32 deps (`display.cpp`, `wifi_manager.cpp`, `ntp.cpp`, `persistence.cpp`, `web_routes.cpp`, `data_fetcher.cpp`) cannot be host-tested — validated on-device only.

## Adding a New Slot (all 7 steps required)

1. Index constant in `config.h`
2. `slotEnabled[]` + `slotIntervalMs[]` entry in `globals.cpp`
3. Cache struct in `globals.h/cpp`
4. Fetch logic in `data_fetcher.cpp` called from `fetcherTick()`
5. Render branch in `slotRotationTick()` in `display.cpp`
6. Config fields in `POST /api/config` in `web_routes.cpp`
7. Persist fields in `persistence.cpp`

## Adding a New UI Language

1. Add code to `_uiLanguages[]` in `persistence.cpp` (single validation source)
2. Add I18N dictionary entry in `web_page.cpp`
No other branching needed.

## Pending Enhancements (docs/enhancements-plan.md)

| Sub-Task | Feature | Notes |
|---|---|---|
| ST-3 | HTTP Basic Auth | Browser sends header automatically; custom headers in JS don't |
| ST-6c | OTA update | Use `Update.h` (built into ESP32 core); `POST /api/ota` receives `.bin` |
| ST-6f | Soft reboot | `scheduleRestart(1500)` exists; just needs a route in `web_routes.cpp` |
| ST-7 | 12-hour clock | New `CLOCK_MODE_HHMMAMPM` constant needed |
| ST-9 | Async WiFi scan | Current `WiFi.scanNetworks()` blocks ~2-3s — non-blocking variant required |
