# AGENTS.md — Plan mode

This file provides guidance to agents planning or designing changes in this repository.

## Architectural Constraints

- **Single cooperative loop** — any new feature must integrate as a non-blocking tick function called from `loop()`. There is no RTOS, no threads, no `delay()` in normal operation.
- **HTTP handlers are stateless mutators** — they may only validate input and write to shared globals. All side effects (display update, network I/O, restart) must be deferred to `loop()`.
- **`HTTPClient` is synchronous** — it blocks for up to 5 s. It must only run from `fetcherTick()`, timed by `millis()`, never inside a handler or render path.
- **Restart requires a deferred pattern** — `scheduleRestart(delayMs)` sets a flag; `loop()` executes `ESP.restart()` after the delay so in-flight HTTP responses complete.
- **Display driver is blocking per-write** — minimize calls to `_display.print()` (the optimisation guard in `displayTick()` exists for this reason).
- **All config persisted in a single NVS namespace** via `Preferences` — plan new fields in `persistence.cpp`, not as separate namespaces.

## Phase Dependencies

```
Phase 1 (clock, NTP, display)       ← DONE
  └─ Phase 2 (persistence, WiFi AP, robust boot)
       └─ Phase 3 (web UI, REST API, full clock with date scroll)
            ├─ Phase 4 (weather slot — Open-Meteo)
            └─ Phase 5 (quotes slot — Yahoo Finance)
```
Phases 4 and 5 are independent of each other but both require Phase 3.

## Data Sources (Phases 4 & 5)

- **Weather**: Open-Meteo — free, no API key, endpoint: `https://api.open-meteo.com/v1/forecast?latitude=X&longitude=Y&current_weather=true&daily=temperature_2m_max,temperature_2m_min`
- **Quotes**: Yahoo Finance — `https://query1.finance.yahoo.com/v7/finance/quote?symbols=TICKER1,TICKER2`; fields: `regularMarketPrice`, `regularMarketChangePercent`.

## Slot Model

- Slot 0 = clock (permanent base, never skipped, `slotIntervalMs[0] = 0`)
- Slot 1 = alert (one-shot, `slotIntervalMs[1] = 0`, consumed and cleared after one scroll)
- Slot 2 = weather (`slotIntervalMs[2]` configurable, default 30 s)
- Slot 3 = quotes (`slotIntervalMs[3]` configurable, default 30 s)
- Disabled slots → silently skipped with no wait
- No-cache slots → silently skipped that cycle; cache-stale slots → shown with `*` prefix
