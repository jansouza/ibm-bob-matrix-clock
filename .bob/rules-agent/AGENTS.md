# AGENTS.md — Agent (coding) mode

This file provides guidance to agents when writing or modifying code in this repository.

## Validation Command

Always run this after any code change to verify the build compiles cleanly before reporting completion:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

## Hard Rules (violating these breaks things)

- **No `delay()` in `loop()`** — use `millis()`-based timers exclusively.
- **No `ESP.restart()` directly** — always set a pending-restart flag/timestamp and execute from `loop()` via `scheduleRestart()`.
- **No display or network I/O inside HTTP handlers** — handlers only validate input and write state variables.
- **No `HTTPClient` inside HTTP handlers** — only call it from `fetcherTick()` in `loop()`.
- **Always convert UTF-8 → Latin-1** before passing strings to `MD_Parola`/`MD_MAX72XX`.
- **Always use `ArduinoJson`** for building or parsing JSON — never string concatenation.

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
- `extern` globals in `globals.h`: no prefix, plain `camelCase` (e.g., `ntpSynced`, `alertPending`).

## Include Order (per file)

1. Own header (e.g., `#include "display.h"`)
2. Project headers (e.g., `#include "config.h"`, `#include "globals.h"`)
3. Arduino/ESP32 library headers (e.g., `#include <MD_Parola.h>`, `#include <Arduino.h>`)
4. C standard headers (e.g., `#include <time.h>`)

## Display Write Optimisation

`displayTick()` compares the new time string against `_lastTimeStr` and only calls `_display.print()` when the content actually changed — preserve this guard when modifying the render path.
