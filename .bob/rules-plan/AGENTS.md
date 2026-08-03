# AGENTS.md — Plan mode

This file provides guidance to agents planning changes or new features in this repository.

## Architectural Constraints

- **`loop()` is single-threaded and must never block** — any new feature requiring I/O (HTTP, filesystem) must be deferred through a state variable and executed from a tick function in `loop()`.
- **HTTP handlers are zero-latency** — they only read the request body and write globals. All side-effects (display, network, restart) happen in the next `loop()` iteration.
- **`ESPAsyncWebServer` handlers run on a background task** — accessing shared globals from a handler is safe only for simple reads/writes; never call `_display` or `HTTPClient` from a handler.
- **`WEB_PAGE_HTML` is a monolithic string** — no templating or server-side rendering. All UI state is fetched via `fetch('/api/config')` and `fetch('/api/status')` on page load. Plan new UI features as additions to the existing JS, not as HTML-returning endpoints.
- **NVS key length limit**: ESP32 `Preferences` key names must be ≤15 characters. All keys are in `config.h` — add new ones there.

## Slot Architecture

- Four slots: 0=clock (permanent base), 1=message (one-shot), 2=weather (timed), 3=quotes (timed).
- Adding a 5th slot requires all 7 steps together: `config.h` index, `globals.cpp` array entries, cache struct, fetcher logic, render branch, REST field, NVS persist — a missing step causes out-of-bounds access.
- Scheduling (`slotScheduleStartMin/EndMin/DaysMask`) only wired for slots ≥2. Slots 0 and 1 ignore these arrays by convention.

## Message Mode Architecture

Four modes defined in `config.h`:
- `MESSAGE_MODE_SCROLL` (0): scroll text, one-shot
- `MESSAGE_MODE_BLINK` (1): blink text for `messageDurationMs`
- `MESSAGE_MODE_STATIC` (2): show static text for `messageDurationMs`
- `MESSAGE_MODE_BLINK_SCROLL` (3): blink phase (fixed `MESSAGE_BLINK_SCROLL_PHASE1_MS`=5s), then scroll; repeating cycle — `messageDurationMs` is the total cycle time

When adding a new message mode: add constant in `config.h`, handle in `displayTick()` in `display.cpp`, and expose via `POST /api/message` in `web_routes.cpp`.

## Language / Localisation

- **Two completely separate systems** (commonly confused):
  - `cfgLocale` = on-device content locale (day/month names via `locale_data.cpp`, number formatting, quote search language)
  - `cfgUiLanguage` = web panel UI language, validated against `_uiLanguages[]` in `persistence.cpp`
- There is **no `cfgLanguage`** variable — older docs that say so are wrong.
- Adding a new UI language: 1 entry in `_uiLanguages[]` + 1 I18N dictionary in `web_page.cpp`. No if/else branching.
- Adding a new display locale: entries in `locale_data.cpp` day/month tables AND the IANA→POSIX TZ table if new timezone regions are needed.

## Display Render Pipeline

```
HTTP handler writes globals
        ↓
displayTick() called every loop()
        ↓
_lastTimeStr diff check → skip if unchanged (MAX7219 write is expensive)
        ↓
CLOCK_MODE_HHMM:   _display.print(buf, PA_CENTER)
CLOCK_MODE_HHMMSS: _display.print(HH:MM, PA_LEFT) → _ssLayout() → setColumn() overlay
```

- `_display.print()` **clears the entire display** — any overlay via `setColumn()` must happen **after** the print, not before.
- Two-pass rendering (print then overlay) is the only viable approach for mixed fonts on MD_Parola; there is no multi-region API.
- Seconds column is computed dynamically by `_ssLayout(hmWidthPx)` — never fixed.

## Text Pipeline (message → display)

```
raw UTF-8 from HTTP body
        ↓
utf8ToLatin1(src, dst)      ← must be first
        ↓
expandIconTags(dst, dst)    ← must be second (ASCII brackets survive encoding)
        ↓
messageText[] (Latin-1)     ← stored here, read by displayTick()
```

Reversing the order corrupts multi-byte characters that happen to contain `[` or `]` bytes.

## Pending Features (docs/enhancements-plan.md)

| Sub-Task | Feature | Key Constraint |
|---|---|---|
| ST-3 | HTTP Basic Auth | Browser sends header automatically; `fetch()` in the panel's JS also sends it without changes |
| ST-6c | OTA update | Use `Update.h` (built into ESP32 core); `POST /api/ota` receives `.bin`; no external lib needed |
| ST-6f | Soft reboot | `scheduleRestart(1500)` already exists; only needs a new route in `web_routes.cpp` |
| ST-7 | 12-hour clock | New `CLOCK_MODE_HHMMAMPM` constant; update `_handleGetStatus()` for `time_str` format |
| ST-9 | Async WiFi scan | `WiFi.scanNetworks()` blocks ~2-3s — must use non-blocking async API, not from `loop()` directly |

## Yahoo Finance

Per-symbol requests only — `v8/finance/chart/{symbol}`. No batch endpoint available without session auth. Each symbol = one HTTP round-trip in `fetcherTick()`.
