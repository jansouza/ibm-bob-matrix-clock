# AGENTS.md — Plan mode

This file provides guidance to agents planning or designing changes in this repository.

## Architectural Constraints

- **Single cooperative loop** — any new feature must integrate as a non-blocking tick function called from `loop()`. There is no RTOS, no threads, no `delay()` in normal operation. Exception: `delay(200)` in `_stationConnect()` (`wifi_manager.cpp`) is intentional and runs only during `setup()`.
- **HTTP handlers are stateless mutators** — they may only validate input and write to shared globals. All side effects (display update, network I/O, restart) must be deferred to `loop()`.
- **`HTTPClient` is synchronous** — it blocks for up to 5 s. It must only run from `fetcherTick()`, timed by `millis()`, never inside a handler or render path. Use `http.getString()` (not `getStream()`) — chunked responses are unreliable with the stream path on ESP32.
- **Restart requires a deferred pattern** — `scheduleRestart(delayMs)` sets a flag; `loop()` executes `ESP.restart()` after the delay so in-flight HTTP responses complete.
- **Display driver is blocking per-write** — minimize calls to `_display.print()` (the optimisation guard in `displayTick()` exists for this reason).
- **All config persisted in a single NVS namespace `"clk"`** via `Preferences` — plan new fields in `persistence.cpp`, not as separate namespaces.
- **`applyTimezone()` must run after `ntpBegin()`** in `setup()` — `configTime()` resets TZ to UTC internally. This ordering is load-bearing; inverting it means the device always runs UTC. `applyTimezone()` must also be re-called in `ntpTick()` after each periodic re-sync.
- **FC16_HW column direction** — raw column 0 is the rightmost physical pixel. Any layout that touches `getGraphicObject()` directly must account for this reversal.

## Phase Status

All 5 phases are complete:

```
Phase 1 (clock, NTP, display)            ✓ Done
Phase 2 (persistence, WiFi AP, boot)     ✓ Done
Phase 3 (web UI, REST API, date/alert)   ✓ Done
Phase 4 (weather slot — Open-Meteo)      ✓ Done
Phase 5 (quotes slot — Yahoo Finance)    ✓ Done
```

Source lives in `smart-matrix-clock-esp32/` subdirectory. Two build systems are available: arduino-cli and PlatformIO (`platformio.ini`). Note: `platformio.ini` references older library versions than arduino-cli; arduino-cli installed versions are the canonical ones.

## Data Sources

- **Weather**: Open-Meteo — free, no API key, `http://api.open-meteo.com/v1/forecast?...&current=temperature_2m,weathercode&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto`
- **Quotes**: Yahoo Finance `v8/finance/chart/{symbol}` — per-symbol (not batch). **Not** `v7/finance/quote` (that endpoint 401s without a session cookie). Requires fake desktop User-Agent. `changePercent` is computed client-side as `(last - prevClose) / prevClose * 100`.

## Slot Model

- Slot 0 = clock (permanent base, never skipped, `slotIntervalMs[0] = 0`)
- Slot 1 = alert (one-shot, `slotIntervalMs[1] = 0`, consumed and cleared after one scroll)
- Slot 2 = weather (`slotIntervalMs[2]` configurable, runtime default from `globals.cpp`: 60 s)
- Slot 3 = quotes (`slotIntervalMs[3]` configurable, runtime default from `globals.cpp`: 120 s)
- Disabled slots → silently skipped with no wait
- No-cache slots → silently skipped that cycle; cache-stale slots → shown with `*` prefix

## Language / Locale Design

There are **two independent language settings**:
- `cfgLanguage` — on-device clock/date display locale (weekday/month names via `locale_data.cpp`)
- `cfgUiLanguage` — web panel UI language (I18N dictionaries in `web_page.cpp`)

Adding a new UI language requires: (1) adding to `_uiLanguages[]` in `persistence.cpp`, (2) adding I18N dictionary in `web_page.cpp`. No other code needs to branch on language.

## Authentication

No authentication exists on any endpoint. An X-API-Key mechanism was implemented and removed — the web panel's JS never sent the header, locking the panel out of its own POST routes. HTTP Basic Auth (covering the whole panel) is the planned replacement per `docs/enhancements-plan.md` Sub-Task 3.
