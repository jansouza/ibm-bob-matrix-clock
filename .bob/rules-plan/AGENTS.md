# AGENTS.md — Plan mode

This file provides guidance to agents planning changes or new features in this repository.

## Architectural Constraints

- **`loop()` is single-threaded and must never block** — any new feature that requires I/O (HTTP, filesystem) must be deferred through a state variable and executed from a tick function in `loop()`.
- **HTTP handlers are zero-latency** — they only read the request body and write globals. All side-effects (display, network, restart) happen in the next `loop()` iteration.
- **`ESPAsyncWebServer` handlers run on a background task** — accessing shared globals from a handler is safe only for simple reads/writes; never call `_display` or `HTTPClient` from a handler.
- **`WEB_PAGE_HTML` is a monolithic string** — there is no templating or server-side rendering. All UI state is fetched via `fetch('/api/config')` and `fetch('/api/status')` on page load. Plan new UI features as additions to the existing JS, not new endpoints that return HTML fragments.
- **NVS key length limit**: ESP32 `Preferences` key names must be ≤15 characters. All NVS keys are defined in `config.h` — add new ones there.

## Slot Architecture

- Four slots: 0=clock (permanent base), 1=message (one-shot), 2=weather (timed), 3=quotes (timed).
- Adding a 5th slot requires: `config.h` index, `globals.cpp` array entries, cache struct, fetcher logic, render branch, REST field, NVS persist — all 7 steps must be done together or the slot index will be out-of-bounds.
- Scheduling (`slotScheduleStartMin/EndMin/DaysMask`) is only wired for slots ≥2. Slots 0 and 1 ignore these arrays by convention.

## Display Render Pipeline

```
HTTP handler writes globals
        ↓
displayTick() called every loop()
        ↓
_lastTimeStr diff check → skip if unchanged (expensive MAX7219 write)
        ↓
CLOCK_MODE_HHMM:   _display.print(buf, PA_CENTER)
CLOCK_MODE_HHMMSS: _display.print(HH:MM, PA_LEFT) → _ssLayout() → setColumn() overlay
```

- `_display.print()` **clears the entire display** — any overlay via `setColumn()` must happen **after** the print, not before.
- Two-pass rendering (print then overlay) is the only viable approach for mixed fonts on MD_Parola; there is no multi-region API.

## Text Pipeline (message → display)

```
raw UTF-8 from HTTP body
        ↓
utf8ToLatin1(src, dst)      ← must be first
        ↓
expandIconTags(dst, dst)    ← must be second (tags use ASCII brackets that survive encoding)
        ↓
messageText[] (Latin-1)     ← stored here, read by displayTick()
```

Reversing the order corrupts multi-byte characters that happen to contain `[` or `]` bytes.

## Language / Localisation

- **Two separate systems**: `cfgLanguage` (display locale, validated against `locale_data.cpp` language tables) and `cfgUiLanguage` (web panel language, validated against `_uiLanguages[]` in `persistence.cpp`).
- Adding a new UI language: 1 entry in `_uiLanguages[]`, 1 I18N dictionary in `web_page.cpp`. No if/else branching.
- Adding a new display locale: requires entries in `locale_data.cpp` day/month tables AND the IANA→POSIX TZ table if new timezone regions are needed.

## Pending Features (enhancements-plan.md)

| Feature | Sub-Task | Key constraint |
|---|---|---|
| HTTP Basic Auth | ST-3 | Must cover panel's own `fetch()` calls — browser sends header automatically; custom headers don't |
| WiFi scan | ST-5 | `WiFi.scanNetworks()` blocks ~2–3 s — only trigger on explicit button press, never from `loop()` |
| Auto brightness | ST-6b | New `displayTick()` check; add `cfgNightBrightnessEnabled/Start/End/Level` globals + NVS keys |
| OTA update | ST-6c | Use `Update.h` (built into ESP32 core); endpoint receives `.bin` via `POST /api/ota`; no external lib needed |
| Soft reboot | ST-6f | `scheduleRestart(1500)` already exists in `wifi_manager.cpp`; only needs a new route in `web_routes.cpp` |

## Yahoo Finance

Per-symbol requests only — `v8/finance/chart/{symbol}`. No batch endpoint available without session auth. Each symbol = one HTTP round-trip in `fetcherTick()`.
