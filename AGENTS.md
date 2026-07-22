# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project

ESP32 firmware (Arduino/arduino-cli) for a MAX7219 LED matrix clock. Currently at Phase 1 of 5 implementation phases — see [`docs/implementation-plan.md`](docs/implementation-plan.md) for the roadmap and [`docs/project-spec.md`](docs/project-spec.md) for the full spec.

## Build & Flash Commands

Board FQBN: `esp32:esp32:esp32` (ESP32 Dev Module)

```bash
# Compile / verify (no device required — use this to check code before flashing)
arduino-cli compile --fqbn esp32:esp32:esp32 .

# Upload to device (replace /dev/ttyUSB0 with the actual port)
arduino-cli upload  --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .

# Compile + upload in one step
arduino-cli compile --fqbn esp32:esp32:esp32 . && \
arduino-cli upload  --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .

# Serial monitor (115200 baud)
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200

# List connected boards (to find the right port)
arduino-cli board list
```

There are no unit tests — validation is done on physical hardware. Always run `arduino-cli compile` before uploading to catch build errors early.

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

- **No blocking anywhere except `setup()`** — `delay()` is forbidden in `loop()`. Every timer uses `millis()`.
- **HTTP handlers must only set state variables** — never call display or network I/O. The actual work happens in `loop()`.
- **`ESP.restart()` is never called directly** — always via `scheduleRestart(delayMs)` (Phase 2+) so the HTTP response reaches the client first.
- **`HTTPClient` is only called from `fetcherTick()`** — never inside HTTP handlers, because it is synchronous and would block the render loop.
- **All display strings must be UTF-8 → Latin-1 converted** before passing to `MD_Parola` / `MD_MAX72XX`.
- **JSON always built/parsed with `ArduinoJson`** — never manual string concatenation.
- **Alert slot is one-shot**: `alertPending` is set by the HTTP handler and cleared by `displayTick()` after one scroll completes. Only the most recent alert is kept.
- **Slot 0 (clock) is the permanent base** — it has no interval (`slotIntervalMs[0] = 0`); other slots return to it after their scroll ends.

## Module Layout

| File | Responsibility |
|---|---|
| `config.h` | All pin definitions, buffer sizes, default values, NVS keys |
| `globals.h/cpp` | Shared state between HTTP layer and display layer (extern vars) |
| `display.h/cpp` | MD_Parola rendering, blink, scroll, slot rotation manager |
| `wifi_manager.h/cpp` | WiFi connect, setup AP, deferred restart |
| `ntp.h/cpp` | `configTime()` wrapper, sync polling, periodic re-sync |
| `text_encoding.h/cpp` | UTF-8 ↔ Latin-1 (Phase 2+) |
| `locale_data.h/cpp` | Day/month names by language, IANA timezone → POSIX TZ table (Phase 2+) |
| `persistence.h/cpp` | NVS load/save via `Preferences`, `applyTimezone()`, `factoryReset()` (Phase 2+) |
| `web_page.h` | Single self-contained HTML/CSS/JS string literal (Phase 3+) |
| `web_routes.h/cpp` | `ESPAsyncWebServer` route registration (Phase 3+) |
| `data_fetcher.h/cpp` | Open-Meteo + Yahoo Finance HTTP fetch, cache structs (Phase 4+) |

## Key Globals (globals.h)

- `ntpSynced` — set true by `ntpTick()`, read by `displayTick()` to switch `--:--` → `HH:MM`
- `alertPending` / `alertMessage[]` — set by HTTP handler, consumed (and cleared) by `displayTick()`
- `slotEnabled[4]` — `{clock, alert, weather, quotes}` — disabled slots are silently skipped
- `slotIntervalMs[4]` — clock slot is `0` (permanent base); alert slot is `0` (one-shot)

## Hardware

- 4× MAX7219 FC16 8×8 LED modules chained via VSPI: CLK=18, DATA/MOSI=23, CS=5
- Display type constant: `MD_MAX72XX::FC16_HW` — must match physical hardware type or columns will be scrambled
- BOOT button (GPIO 0) held at power-on triggers factory reset (Phase 2+)

## NTP / Timezone

- `configTime(0, 0, server)` always sets UTC offsets to 0 — timezone is handled entirely via POSIX TZ string through `setenv("TZ", ...)` + `tzset()`
- Clock validity check: `tm_year + 1900 >= 2020` (year < 2020 means epoch, i.e., not synced)
- Periodic re-sync calls `configTime()` again; `ntpSynced` stays `true` during re-sync (clock keeps last value)

## Failure / Cache Policy (Phase 4+)

- If a slot's data fetch fails and there is no cache → slot is silently skipped that rotation cycle
- If cache exists but fetch failed → display with `*` prefix (e.g., `*22°C Nublado`)
