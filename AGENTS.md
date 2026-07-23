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

There are no unit tests — validation is done on physical hardware. Always run the compile/verify step after any code change before reporting completion.

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
- **All display strings must be UTF-8 → Latin-1 converted** before passing to `MD_Parola` / `MD_MAX72XX`. `alertMessage[]` is stored in Latin-1, not UTF-8.
- **Alert pipeline order matters**: `utf8ToLatin1()` first, then `expandIconTags()` — icon tags contain ASCII-range brackets that survive the encoding step.
- **JSON always built/parsed with `ArduinoJson`** — never manual string concatenation.
- **Alert slot is one-shot**: `alertPending` is set by the HTTP handler and cleared by `displayTick()` after one scroll completes. Only the most recent alert is kept.
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
- `alertPending` / `alertMessage[]` — set by HTTP handler, consumed (and cleared) by `displayTick()`
- `alertBrightness` / `alertScrollSpeedMs` — per-alert overrides; `-1` means use configured global value
- `slotEnabled[4]` — `{clock, alert, weather, quotes}` — disabled slots are silently skipped
- `slotIntervalMs[4]` — clock slot is `0` (permanent base); alert slot is `0` (one-shot); default values in `globals.cpp` are `{0, 0, 60000, 120000}` regardless of `config.h` display defaults

## Hardware

- 4× MAX7219 FC16 8×8 LED modules chained via VSPI: CLK=18, DATA/MOSI=23, CS=5
- Display type constant: `MD_MAX72XX::FC16_HW` — must match physical hardware type or columns will be scrambled
- **FC16_HW column direction**: raw column 0 is the **rightmost** physical pixel, raw column 31 is the leftmost — critical for any direct pixel manipulation via `getGraphicObject()`
- BOOT button (GPIO 0, active-LOW) held at power-on triggers factory reset

## NTP / Timezone

- `configTime(0, 0, server)` always sets UTC offsets to 0 — timezone is handled entirely via POSIX TZ string through `setenv("TZ", ...)` + `tzset()`
- Clock validity check: `tm_year + 1900 >= 2020` (year < 2020 means epoch, i.e., not synced)
- Periodic re-sync calls `configTime()` again; `ntpSynced` stays `true` during re-sync (clock keeps last value); `applyTimezone()` must be re-applied immediately after or the display reverts to UTC

## Data Sources

- **Weather**: Open-Meteo — free, no API key; uses `current=temperature_2m,weathercode` + `daily=temperature_2m_max,temperature_2m_min` in a single request
- **Quotes**: Yahoo Finance `v8/finance/chart/{symbol}` (per-symbol, not batch) — **not** `v7/finance/quote` (that endpoint 401s without a session cookie). A fake desktop User-Agent header is required; the ESP32 default UA is rejected.

## Failure / Cache Policy

- If a slot's data fetch fails and there is no cache → slot is silently skipped that rotation cycle
- If cache exists but fetch failed → display with `*` prefix (e.g., `*22°C Nublado`)
