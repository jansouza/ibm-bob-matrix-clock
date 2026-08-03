# AGENTS.md — Agent (coding) mode

This file provides guidance to agents when writing or modifying code in this repository.

## Validation Command

Always run both after any code change before reporting completion:

```bash
# 1. Host tests — must finish ✓ ALL PASS
cd tests && make

# 2. Firmware compile — no device required
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
```

**Source lives in `smart-matrix-clock-esp32/` — not `.`.**

## Hard Rules (violating these breaks things)

- **No `delay()` in `loop()`** — `millis()`-based timers only. Exception: `delay(200)` in `_stationConnect()` (`wifi_manager.cpp`) runs during `setup()` only.
- **No `ESP.restart()` directly** — use `scheduleRestart()` (in `wifi_manager.cpp`) so the HTTP response reaches the client first.
- **HTTP handlers only set state variables** — no display or network I/O.
- **`HTTPClient` only from `fetcherTick()`** — never inside HTTP handlers.
- **`http.getString()`, not `http.getStream()`** — chunked/compressed responses on ESP32 are unreliable with `getStream()`, causing spurious JSON parse errors.
- **UTF-8 → Latin-1 then `expandIconTags()`** — order is mandatory. `messageText[]` is stored Latin-1.
- **`ArduinoJson` for all JSON** — never string concatenation.
- **`applyTimezone()` must follow `ntpBegin()`** in `setup()`, and again in `ntpTick()` on every re-sync — `configTime()` resets TZ to UTC internally every call.

## Variable Name Pitfall

- `cfgLocale` = on-device content locale (date names, number format, quote search lang) — validated by `locale_data.cpp`
- `cfgUiLanguage` = web panel UI language — validated by `_uiLanguages[]` in `persistence.cpp`
- **There is no `cfgLanguage`** — any docs saying otherwise are stale.

## Clock Mode Gotchas

- In `CLOCK_MODE_HHMMSS`: `HH:MM` is rendered with `PA_LEFT`; seconds are overlaid via `setColumn()` **after** the print. The seconds start column is **dynamic** — `_ssLayout(hmWidthPx)` measures `HH:MM` width first. Never hardcode a column.
- `_writeSmallDigit(visualCol, digit)` reverses: `setColumn(31 - visualCol - i, ...)` to compensate for FC16_HW's right-to-left raw column order.
- Runtime `cfgClockMode` change: must call `displayForceRedraw()` or the display retains stale alignment state.
- `/api/status` `time_str` must reflect the active clock mode — if adding a mode, update `_handleGetStatus()` in `web_routes.cpp`.

## Message Modes

Four modes in `config.h`: `MESSAGE_MODE_SCROLL` (0), `MESSAGE_MODE_BLINK` (1), `MESSAGE_MODE_STATIC` (2), `MESSAGE_MODE_BLINK_SCROLL` (3). In `BLINK_SCROLL` mode, `messageDurationMs` is the total for the entire repeating blink→scroll cycle (not per-phase).

## Slot Scheduling

- `slotScheduleStartMin[slot] == slotScheduleEndMin[slot]` → **always active** (not an invalid range).
- `slotScheduleDaysMask[slot]` uses `tm_wday` bits: 0=Sunday … 6=Saturday. `0x7F` = all days.
- Scheduling only wired for slots 2 (weather) and 3 (quotes).

## UI Language Validation

`_uiLanguages[]` in `persistence.cpp` is the **single source of truth**. Add code there first, then add I18N dictionary in `web_page.cpp`. No other branching.

## Message History Ring Buffer

- `messageHistory[]` (`globals.h`): 20-entry ring buffer, `messageHistoryHead` = oldest index.
- Iterate oldest-to-newest: `idx = (messageHistoryHead + i) % MESSAGE_HISTORY_SIZE` for `i` in `0..messageHistoryCount-1`.

## Test Suite — Adding Tests

- Add `test_<name>.cpp` to `tests/`, register in `TEST_SRCS` in `tests/Makefile`.
- For `static` (private) functions: copy the algorithm into the test file — comment `// Algorithm replica from <file>.cpp / <function>()`.
- `tests/stubs/arduino_stub.h` provides `PROGMEM`, `pgm_read_byte`, and `Serial` stubs — add only what new tests actually need.

## Adding a New Slot (all 7 steps required together)

1. Index constant in `config.h`
2. `slotEnabled[]` + `slotIntervalMs[]` in `globals.cpp`
3. Cache struct in `globals.h/cpp`
4. Fetch logic in `data_fetcher.cpp` → `fetcherTick()`
5. Render branch in `slotRotationTick()` in `display.cpp`
6. Config fields in `POST /api/config` in `web_routes.cpp`
7. Persist in `persistence.cpp`

## Naming Conventions

- Module-private state: `static` in `.cpp`, prefixed `_` (e.g., `_lastBlink`)
- Public API: `camelCase` with module prefix (e.g., `displayTick()`, `ntpBegin()`)
- Constants in `config.h`: `SCREAMING_SNAKE_CASE`
- `extern` globals in `globals.h`: no prefix, plain `camelCase`
- NVS keys in `config.h`: ≤15 characters (hard ESP32 `Preferences` limit)

## Include Order (per file)

1. Own header
2. Project headers (`config.h`, `globals.h`, etc.)
3. Arduino/ESP32 library headers
4. C standard headers

## FC16_HW Column Direction (direct pixel work only)

Raw column 0 = rightmost pixel, raw column 31 = leftmost. Read the comment in `_startDateDisplay()` in `display.cpp` before doing any `getGraphicObject()` pixel work.

## Yahoo Finance API

`v8/finance/chart/{symbol}` per-symbol — **not** `v7/finance/quote` (401s without session cookie). Requires `http.setUserAgent(...)` with a fake desktop UA.
