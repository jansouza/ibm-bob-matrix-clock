# AGENTS.md — Agent (coding) mode

This file provides guidance to agents when writing or modifying code in this repository.

## Validation Command

Always run this after any code change to verify the build compiles cleanly before reporting completion:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
```

**Source lives in the `smart-matrix-clock-esp32/` subdirectory — not in `.`.**

## Hard Rules (violating these breaks things)

- **No `delay()` in `loop()`** — use `millis()`-based timers exclusively. Exception: `delay(200)` in `_stationConnect()` (`wifi_manager.cpp`) is intentional; it runs only during `setup()`.
- **No `ESP.restart()` directly** — always set a pending-restart flag/timestamp and execute from `loop()` via `scheduleRestart()`.
- **No display or network I/O inside HTTP handlers** — handlers only validate input and write state variables.
- **No `HTTPClient` inside HTTP handlers** — only call it from `fetcherTick()` in `loop()`.
- **Use `http.getString()`, not `http.getStream()`** — chunked/compressed ESP32 responses are unreliable with `getStream()`, producing spurious `InvalidInput` JSON parse errors on valid payloads.
- **Always convert UTF-8 → Latin-1** before passing strings to `MD_Parola`/`MD_MAX72XX`, then call `expandIconTags()` after — order matters. `alertMessage[]` is stored Latin-1.
- **Always use `ArduinoJson`** for building or parsing JSON — never string concatenation.
- **`applyTimezone()` must run after `ntpBegin()`** in `setup()` — and again in `ntpTick()` after each periodic re-sync — because `configTime()` resets TZ to UTC internally every call.

## Adding a New Slot

1. Add an index constant in `config.h`.
2. Initialize `slotEnabled[]` and `slotIntervalMs[]` entry in `globals.cpp`.
3. Add cache struct in `globals.h/cpp`.
4. Add fetch logic in `data_fetcher.cpp` called from `fetcherTick()`.
5. Add rendering branch in `slotRotationTick()` in `display.cpp`.
6. Expose enable/interval config fields through `POST /api/config` in `web_routes.cpp`.
7. Persist the new fields in `persistence.cpp`.

## Adding a New Web UI Language

1. Add the code string to `_uiLanguages[]` in `persistence.cpp` (this is the single validation source).
2. Add a matching I18N dictionary entry in `web_page.cpp`.
No other branching logic is needed.

## Naming Conventions

- Module-private state variables: `static` in `.cpp`, prefixed with `_` (e.g., `_lastBlink`, `_colonVisible`).
- Public API functions: `camelCase` with module prefix (e.g., `displayTick()`, `ntpBegin()`, `wifiConnect()`).
- Constants in `config.h`: `SCREAMING_SNAKE_CASE`.
- `extern` globals in `globals.h`: no prefix, plain `camelCase` (e.g., `ntpSynced`, `alertPending`).

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
