# Implementation Plan — Smart Matrix Clock

## Overview

Firmware implementation in **5 incremental phases**, each delivering a functional and hardware-testable product. Each phase adds layers on top of the previous one without rewriting what already works.

Reference: [`docs/project-spec.md`](docs/project-spec.md)

---

## Target file structure

```
smart-matrix-clock-esp32/
├── smart-matrix-clock-esp32.ino ← main sketch (setup + loop)
├── config.h               ← pins, constants, limits, defaults, NVS keys
├── globals.h / globals.cpp ← shared state between modules
├── text_encoding.h / .cpp ← UTF-8 → Latin-1 / Latin-1 → UTF-8
├── locale_data.h / .cpp   ← day/month names by language, IANA→POSIX table
├── persistence.h / .cpp   ← NVS load/save, apply timezone
├── display.h / .cpp       ← rendering, scroll, blink, slot manager
├── wifi_manager.h / .cpp  ← WiFi connection, setup AP, deferred restart
├── ntp.h / .cpp           ← NTP sync, periodic re-sync
├── web_page.h             ← HTML/CSS/JS page string literal
├── web_routes.h / .cpp    ← HTTP route registration and handlers
└── data_fetcher.h / .cpp  ← external HTTP fetch (Weather, Quotes), cache
```

---

## Phase 1 — Project skeleton + functional clock

**Objective:** have an ESP32 showing the correct time on the LED display, synchronised via NTP, with the timezone configured in code, without configurable WiFi yet.

**Verifiable deliverable:** the display shows `HH:MM` with a blinking colon; after connecting to the hardcoded network, it syncs NTP and shows the correct local time. Before sync it shows `--:--`.

### Sub-tasks

#### 1.1 — Scaffolding and Config
- Create the main sketch `smart-matrix-clock-esp32.ino` with empty `setup()` and `loop()`.
- Create [`config.h`](smart-matrix-clock-esp32/config.h) with: SPI pins (CLK, DATA, CS), number of modules (4), buffer size constants, default brightness and scroll speed values.
- Add dependencies in `platformio.ini` or `libraries.txt`: `MD_MAX72XX`, `MD_Parola` (or equivalent), `ESPAsyncWebServer`, `ArduinoJson`, `Preferences` (ESP32 built-in).

**Status:** `[x] done`

#### 1.2 — Globals
- Create [`globals.h`](../globals.h) and [`globals.cpp`](../globals.cpp) declaring the shared state variables: `currentBrightness`, `scrollSpeed`, `ntpSynced`, `alertPending`, `alertMessage[]`, `activeSlot`, `slotEnabled[]`, `slotIntervalMs[]`.
- All variables `extern` in the `.h`, defined in the `.cpp`.

**Status:** `[x] done`

#### 1.3 — Display module (basic rendering)
- Create [`display.h`](../display.h) / [`display.cpp`](../display.cpp).
- Initialise `MD_Parola` (over `MD_MAX72XX`) with the pins from `config.h`.
- Implement `displayTick()` — called in `loop()` on each iteration, manages:
  - Colon blink every 500 ms via `millis()`.
  - Show `--:--` when `ntpSynced == false`.
  - Show `HH:MM` when synced.
- Implement `setBrightness(uint8_t)` and `setScrollSpeed(uint16_t)`.

**Status:** `[x] done`

#### 1.4 — WiFi (simple hardcoded connection) + NTP
- Create [`wifi_manager.h`](../wifi_manager.h) / [`wifi_manager.cpp`](../wifi_manager.cpp) with `wifiConnect(ssid, password, timeoutMs)` — synchronous only in `setup()`, with timeout.
- Create [`ntp.h`](../ntp.h) / [`ntp.cpp`](../ntp.cpp): `ntpBegin(server, timezone_posix)`, `ntpTick()` — checks if `configTime` has synced and sets `ntpSynced = true`; implements periodic re-sync via timer.
- In `setup()`: connect to hardcoded WiFi → call `ntpBegin()`. In `loop()`: call `ntpTick()` and `displayTick()`.
- Timezone fixed in `config.h` for now (`"BRT3"` for Brasília).

**Status:** `[x] done`

---

## Phase 2 — Persistence, robust boot and configurable WiFi

**Objective:** the device remembers settings between reboots and allows WiFi credentials to be configured via a setup Access Point, without relying on hardcoded credentials.

**Verifiable deliverable:** when powered on without saved WiFi, opens `SmartMatrixClock-Setup` AP; after connecting and saving credentials, reboots and connects to the provided network. When powered on with the BOOT button held, wipes everything and returns to the AP.

### Sub-tasks

#### 2.1 — Text encoding and Locale data
- Create [`text_encoding.h`](smart-matrix-clock-esp32/text_encoding.h) / [`text_encoding.cpp`](smart-matrix-clock-esp32/text_encoding.cpp): functions `utf8ToLatin1(src, dst, maxLen)` and `latin1ToUtf8(src, dst, maxLen)`.
- Create [`locale_data.h`](smart-matrix-clock-esp32/locale_data.h) / [`locale_data.cpp`](smart-matrix-clock-esp32/locale_data.cpp): day/month name tables for `pt` and `en`; IANA timezone → POSIX TZ string table for the most common Brazilian timezones + UTC + US/EU (minimum 20 entries).

**Status:** `[x] done`

#### 2.2 — Persistence (NVS)
- Create [`persistence.h`](smart-matrix-clock-esp32/persistence.h) / [`persistence.cpp`](smart-matrix-clock-esp32/persistence.cpp).
- Implement `loadConfig()` — reads all NVS fields with fallback to defaults from `config.h`.
- Implement `saveConfig()` — writes all changed fields.
- Fields: brightness, scroll speed, timezone (IANA name), language, NTP server, date interval, WiFi credentials (SSID + password), lat/lon coordinates, ticker list (serialised), intervals and enabled flags per slot.
- Implement `applyTimezone(ianaName)` — translates IANA name → POSIX via `locale_data` and calls `setenv("TZ", ...)` + `tzset()`.
- Implement `factoryReset()` — wipes the entire NVS namespace.

**Status:** `[x] done`

#### 2.3 — Full WiFi Manager + setup AP
- Expand [`wifi_manager.cpp`](smart-matrix-clock-esp32/wifi_manager.cpp): `wifiBegin()` — reads credentials from NVS, tries to connect with timeout; if it fails, calls `startConfigAP()`.
- `startConfigAP()` — brings up AP with SSID `SmartMatrixClock-Setup`, fixed IP `192.168.4.1`, accepts HTTP connections to configure a new network.
- `wifiTick()` — called in `loop()`, checks reconnection if WiFi drops.
- Implement `scheduleRestart(delayMs)` — sets restart pending flag + timestamp; `loop()` calls `ESP.restart()` after the delay.

**Status:** `[x] done`

#### 2.4 — Complete boot sequence
- In `setup()`: check GPIO 0 pressed → `factoryReset()`; otherwise → `loadConfig()` → `applyTimezone()` → `wifiBegin()` → `ntpBegin()`.
- Integrate `wifiTick()` into `loop()`.

**Status:** `[x] done`

---

## Phase 3 — Web interface + REST API + full clock

**Objective:** the device exposes the full web panel and REST API; the clock shows localised date in periodic scroll; all configuration can be done from the panel.

**Verifiable deliverable:** accessing `http://<ip>/` opens the panel; changing the timezone in the panel changes the time on the display; sending an alert message via the panel shows it on the display; periodic NTP re-sync works.

### Sub-tasks

#### 3.1 — HTML/CSS/JS page
- Create [`web_page.h`](smart-matrix-clock-esp32/web_page.h) with the `WEB_PAGE_HTML` string literal of the self-contained page.
- The page has 4 tabs: **Clock** (timezone, language, NTP, date interval, brightness, scroll, live preview); **Weather** (enable, coordinates, intervals, unit); **Quotes** (enable, tickers, intervals); **Network** (current SSID, new credential form). A 5th **API** tab (auth toggle, show/regenerate key) was implemented and later removed — see `docs/enhancements-plan.md`, Sub-Task 3.
- Live preview in Clock tab: polling `GET /api/status` every 1 s, updates a `<div>` simulating the display.
- All page communication with the ESP32 via `fetch()` to the API endpoints.

**Status:** `[x] done`

#### 3.2 — HTTP Routes
- Create [`web_routes.h`](smart-matrix-clock-esp32/web_routes.h) / [`web_routes.cpp`](smart-matrix-clock-esp32/web_routes.cpp).
- Register with `ESPAsyncWebServer`:
  - `GET /` → serves `WEB_PAGE_HTML`.
  - `GET /api/status` → JSON with current time, active slot, `ntpSynced`, SSID, IP.
  - `GET /api/config` → JSON with all current settings.
  - `POST /api/config` → receives JSON, validates each field, calls `saveConfig()`, applies changes; responds before any reboot.
  - `POST /api/alert` → receives `{"message": "...", "speed": N}`, validates, sets `alertMessage` and `alertPending = true`.
  - `GET /api/timezones` → JSON array with available IANA names.
  - `POST /api/wifi` → saves new credentials and schedules `scheduleRestart(1500)`.
- Validations: brightness ranges (0–15), scroll speed (10–200 ms), language in allowed list, timezone in IANA table, lat/lon in valid ranges (-90..90, -180..180).

**Status:** `[x] done`

#### 3.3 — Display: localised date and Alert message
- Expand [`display.cpp`](smart-matrix-clock-esp32/display.cpp):
  - `displayTick()` — add date display timer: when it fires, builds the string `"MON 14/07"` (using `locale_data` for the configured language) and scrolls it; after the scroll, returns to the time.
  - Implement alert queue: when `alertPending == true` and no scroll is in progress, takes `alertMessage`, clears `alertPending`, executes scroll, then returns to normal flow.
  - Implement non-blocking scroll: each `displayTick()` call advances one scroll frame; the full loop never waits.

**Status:** `[x] done`

---

## Phase 4 — Weather slot (Open-Meteo)

**Objective:** the Weather slot enters the automatic rotation, queries Open-Meteo periodically and shows temperature + condition + min/max in scroll, with cache fallback and `*` indicator.

**Verifiable deliverable:** with Weather slot enabled, after the clock displays for X seconds, the display scrolls `22°C Cloudy Min18 Max27`; when the slot is disabled in the panel, display stops; when simulating network failure, shows `*22°C ...` if there is cache.

### Sub-tasks

#### 4.1 — External Data Fetcher module (base)
- Create [`data_fetcher.h`](smart-matrix-clock-esp32/data_fetcher.h) / [`data_fetcher.cpp`](smart-matrix-clock-esp32/data_fetcher.cpp).
- Implement `fetcherTick()` — called in `loop()`, checks Weather and Quotes timers.
- Implement `fetchWeather()` — builds Open-Meteo URL with lat/lon and fields `temperature_2m,weathercode,temperature_2m_max,temperature_2m_min`; calls `HTTPClient` with 5 s timeout; parses JSON with `ArduinoJson`; updates `weatherCache` in globals; sets flag `weatherCacheValid`.
- Implement `weathercode` → condition string mapping (e.g.: 0→`Clear`, 1→`Cloudy`, 61→`Rain`, etc.) — table in `locale_data` by language.
- Cache: struct `WeatherCache { float temp; float minTemp; float maxTemp; char condition[20]; bool valid; uint32_t fetchedAt; }` in globals.

**Status:** `[x] done`

#### 4.2 — Slot Rotation Manager
- Expand [`display.cpp`](smart-matrix-clock-esp32/display.cpp): implement `slotRotationTick()` — controls which slot is active based on `millis()` and `slotIntervalMs[]`; skips disabled slots; skips slots without valid cache; when switching slots, triggers rendering of the new slot.
- Integrate into `displayTick()`: if the active slot is Weather, build the string `"TEMP°U COND MinX MaxY"` (with `*` prefix if cache is stale) and start scroll. When the scroll finishes, the slot ends and the manager returns to Clock.

**Status:** `[x] done`

#### 4.3 — Weather configuration in the panel and persistence
- Ensure `POST /api/config` accepts and saves: `weather_enabled`, `weather_lat`, `weather_lon`, `weather_update_interval`, `weather_display_interval`, `temp_unit`.
- Ensure `loadConfig()` and `saveConfig()` cover these fields.

**Status:** `[x] done`

---

## Phase 5 — Quotes slot (Yahoo Finance) `[x] done`

**Objective:** the Quotes slot enters the rotation, queries Yahoo Finance periodically and shows `SYMBOL: price change%` in scroll, with cache fallback.

**Verifiable deliverable:** with tickers `PETR4.SA,AAPL` configured, the display scrolls `PETR4: 38.42 +1.23%  AAPL: 189.75 -0.45%`; when disabled, it stops; cache with `*` when simulating failure.

### Sub-tasks

#### 5.1 — Quotes fetch
- Expand [`data_fetcher.cpp`](smart-matrix-clock-esp32/data_fetcher.cpp): implement `fetchQuotes()`.
- Build Yahoo Finance URL for multiple symbols (endpoint `query1.finance.yahoo.com/v7/finance/quote?symbols=...`).
- Parse JSON: for each symbol, extract `regularMarketPrice` and `regularMarketChangePercent`.
- Cache: array of `QuoteCache { char symbol[12]; float price; float changePercent; bool valid; }` structs in globals, maximum size N (defined in `config.h`).
- Build display string: `"SYM1: P1 C1%  SYM2: P2 C2%"` with explicit `+`/`-` sign on the change; `*` prefix if cache is stale.

**Status:** `[x] done`

#### 5.2 — Integration into the slot manager
- Expand `slotRotationTick()`: Quotes slot follows the same pattern as the Weather slot — checks cache, builds string, starts scroll, ends when scroll finishes.
- Ensure `POST /api/config` accepts and saves: `quotes_enabled`, `quotes_tickers` (comma-separated string), `quotes_update_interval`, `quotes_display_interval`.
- Ensure `loadConfig()` and `saveConfig()` cover these fields.

**Status:** `[x] done`

---

## Phase dependencies

```
Phase 1 (Basic clock)
    └── Phase 2 (Persistence + configurable WiFi)
            └── Phase 3 (Web + API + full clock)
                    ├── Phase 4 (Weather slot)
                    └── Phase 5 (Quotes slot)
```

Phases 4 and 5 are independent of each other — they can be implemented in any order after Phase 3.

---

## Library dependencies (PlatformIO / Arduino IDE)

| Library | Usage |
|---|---|
| `MD_MAX72XX` + `MD_Parola` | LED display driver |
| `ESPAsyncWebServer` + `AsyncTCP` | Async HTTP server |
| `ArduinoJson` v6+ | JSON build and parse |
| `Preferences` (ESP32 built-in) | NVS persistence |
| `HTTPClient` (ESP32 built-in) | External API queries |
| `WiFi` (ESP32 built-in) | WiFi connection and AP |
| `time.h` + `configTime` (ESP32 built-in) | NTP sync and timezone |

---

## Implementation conventions

- **No `delay()`** outside `setup()`. All timing via `millis()`.
- **HTTP handlers** only validate input and update state variables — never perform display or network I/O.
- **`HTTPClient`** only called inside `fetcherTick()`, via `loop()`, never inside handlers.
- **JSON** always built/parsed with `ArduinoJson` — never manual string concatenation.
- **Display strings** always converted from UTF-8 → Latin-1 before passing to the driver.
- **Reboots** always via `scheduleRestart()` — never direct `ESP.restart()`.
