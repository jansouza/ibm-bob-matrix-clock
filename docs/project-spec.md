## 1. What it is

Firmware for ESP32 that drives MAX7219/FC16 LED matrix modules (via the
`MD_MAX72XX` library) as a WiFi-connected display. Runs cooperatively (no
RTOS, no blocking calls) inside the Arduino `loop()`. The device exposes a
web interface and a REST API, both served by the ESP32 itself, and persists
configuration to flash (NVS) to survive reboots.

## 2. Main Features

### Rotation model

The clock is the **continuous base mode**: it is displayed by default
whenever no other feature is active. The extra features enter an
**automatic rotation** — each one is displayed for a configurable interval
and, once it ends, the display automatically returns to the clock. The
order and display time of each rotation slot are configurable by the user
in the web panel or via the REST API. All rotation is managed by
non-blocking timers based on `millis()`, without interrupting clock
rendering between transitions.

Slots can be **individually enabled or disabled** in the panel. Disabled
slots are skipped with no wait, as if they didn't exist in the rotation.

**External data failure policy:** weather and quotes data are cached after
each successful query. If a slot is displayed while a query has failed and
a cache is available, the cached data is shown with an asterisk prefix
(`*`) to indicate it may be stale. If there is no cache at all for the slot
(e.g., first boot without connectivity), the slot is **silently skipped**
in that rotation cycle.

---

### 2.1 Clock (base mode)

**Purpose:** continuously display the current local time, serving as an
anchor between all other features.

**Data displayed:**
- Current time in `HH:MM` format, with the colon blinking every half
 second to indicate the device is active and synced.
- Current date (weekday + day/month) displayed periodically via scroll,
 with a configurable interval — stays visible for a few seconds and
 returns to the time display.

**Time sync (NTP):**
- After connecting to WiFi, the device immediately starts syncing with the
 NTP server.
- Default NTP server: `pool.ntp.org` (configurable).
- Periodic re-sync every hour to correct clock drift.
- While waiting for the first successful sync, the display shows `--:--`
 instead of the time.
- If NTP doesn't respond after the initial attempts, the display stays at
 `--:--` and retries at each re-sync interval.

**Configuration:**
- Timezone: selectable by IANA name (e.g., `America/Sao_Paulo`) with
 automatic daylight saving time support.
- Date language: selectable (e.g., Portuguese, English) to localize
 weekday and month names.
- Date display interval: time (in seconds) between each appearance of the
 date screen.
- Display brightness and date scroll speed: configurable.
- NTP server: configurable via the web panel.

---

### 2.2 Alert Message

**Purpose:** allow external systems or the user to send a one-off text
message to the display, for point-in-time notifications (e.g., home
automation alerts, reminders).

**Behavior:**
- Sent via REST API or web panel.
- Upon receipt, it is queued for display — **never interrupts an ongoing
 scroll** to avoid visual corruption.
- As soon as the current scroll finishes, the alert message is displayed
 in a full scroll, exactly once.
- Once the scroll completes, the message is **automatically discarded** —
 it does not enter the periodic rotation and is not kept for
 re-display.
- The display returns to the clock (or the next rotation slot) as soon as
 the alert scroll finishes.

**Concurrency:** if multiple alerts arrive in sequence before being
displayed, only the most recent is kept — previously received but
not-yet-displayed alerts are discarded.

**Configuration:**
- Message scroll speed: configurable per call parameter or via the global
 default.
- Maximum message length: limited to a fixed size (fixed-size buffer,
 with explicit truncation).

---

### 2.3 Temperature and Weather Forecast

**Purpose:** periodically display the user's local weather conditions,
alternating with the clock.

**Data source:** [Open-Meteo](https://open-meteo.com/) — free REST API, no
authentication key required. Queried by the ESP32 itself in the
background, non-blocking.

**Data displayed** (in continuous scroll during the slot):
- Current temperature (e.g., `22°C`).
- Summarized weather condition (e.g., `Cloudy`, `Sunny`, `Light rain`).
- Day's minimum and maximum temperature (e.g., `Min 18°C  Max 27°C`).

**Configuration:**
- Geographic coordinates (latitude and longitude): configurable in the web
 panel — used directly in Open-Meteo API calls.
- Data refresh interval: time between API queries (e.g., every 10
 minutes), to avoid overloading the network.
- Display interval: how long the weather slot stays visible each rotation
 cycle.
- Temperature unit: Celsius (default) or Fahrenheit, configurable.

**Failure behavior:** if the query fails and there is no cache, the slot is
skipped that cycle. If there is a cache, it displays the data with a `*`
prefix (e.g., `*22°C Cloudy Min18 Max27`). New query attempts happen at the
configured interval regardless of whether the slot is active.

---

### 2.4 Asset Quotes

**Purpose:** periodically display prices and change of financial assets
configured by the user (stocks, ETFs, cryptocurrencies with a supported
ticker), alternating with the clock.

**Data source:** Yahoo Finance public API (unauthenticated endpoint) —
queried by the ESP32 itself in the background, non-blocking.

**Data displayed** (in continuous scroll during the slot):
- For each configured asset, the display shows in sequence:
 `SYMBOL: price change%`
 — e.g., `PETR4: 38.42 +1.23%  AAPL: 189.75 -0.45%`
- Assets are displayed in sequence in the same scroll, separated by a
 space.

**Configuration:**
- Symbol list (tickers): up to N symbols defined by the user in the web
 panel (e.g., `PETR4.SA`, `AAPL`, `BTC-USD`). The value of N is limited by
 API response capacity and buffer size.
- Data refresh interval: time between API queries.
- Display interval: how long the quotes slot stays visible each rotation
 cycle.

**Failure behavior:** same policy as the Weather slot — skip with no
cache, display with `*` prefix if a cache is available.

---

### 2.5 Extensibility

The rotation architecture was designed to accommodate new features without
changing the base flow. Adding a new slot to the rotation only requires:
- Defining the data-fetch logic (if any).
- Defining the display string formatting logic.
- Registering the new slot in the rotation manager.

The clock, as the base mode, is never removed from execution — it just
stays in the background while another slot is active.


## 3. Target hardware

- ESP32 development board.
- 4 chained MAX7219 LED matrix modules (FC16 type) of 8×8, forming a
 32-column × 8-row display, via hardware SPI.
- Pins configurable via constants (default: CLK, DATA/MOSI, CS on the
 ESP32's VSPI pins).
- The BOOT button (GPIO 0) also acts as a factory-reset trigger when held
 down at power-on.

## 4. Constraints that drive the design

Think about this from the start — it shapes everything else:

- **No blocking calls in normal operation.** Display updates, scrolling,
 WiFi, HTTP, NTP: all of it must run with non-blocking timers based on
 `millis()`, checked in `loop()`. A stuck handler or a misplaced `delay()`
 freezes the entire display.
- **HTTP handlers must respond quickly.** Use an asynchronous web server
 so requests don't block the render loop. Handlers should only validate
 input, change state variables, and respond; the actual work (updating
 the display, changing WiFi, restarting) happens afterward, in `loop()`.
- **Restarts are deferred, never immediate.** Anything that needs
 `ESP.restart()` (e.g., after saving new WiFi credentials) must set a
 "pending restart" flag/timestamp and let `loop()` perform the restart
 after a short delay — so the HTTP response reaches the client first.
- **Persist only what's needed to survive a restart**, and keep it small:
 brightness, scroll speed, clock settings (timezone, language, NTP
 server, date interval), API key + auth flag, saved WiFi credentials,
 geographic coordinates (lat/lon) for weather, asset ticker list,
 per-slot rotation and external-data-refresh intervals, and each slot's
 enabled/disabled state. Use the platform's key-value storage (e.g.,
 ESP32's `Preferences`/NVS) in a single namespace.
- **Limit all user input.** Fixed-size message buffer, with explicit
 truncation and null termination; validate every REST/UI parameter
 (speed/brightness ranges, allowed timezone/language lists, etc.) before
 changing state.
- **Optional API authentication**, only on the programmatic endpoint — the
 local web interface's own routes remain implicitly trusted (it is the
 configuration surface that manages the key itself). Verify the key
 before any state change.

## 5. Suggested module breakdown

Split by responsibility into distinct files/headers, rather than a single
giant sketch. One viable breakdown (the one used in this project):

- **Config** — hardware pins, size/limit constants, default values, NVS
 key names, list of supported languages.
- **Globals** — state shared between the HTTP layer and the display
 rendering layer (current message, current mode, current settings, "new
 message available" flag, etc.).
- **Text encoding** — messages arrive in UTF-8 (from browsers/HTTP
 clients), but common LED-matrix font libraries only render
 Latin-1/CP437. Convert UTF-8 → Latin-1 on input and the reverse on
 output (e.g., when echoing the current message in an API response).
- **Locale data** — weekday and month names per supported language, and a
 conversion table from IANA timezone name → POSIX TZ string (to
 configure timezones by name, e.g., `America/Sao_Paulo`, with automatic
 daylight saving, instead of a raw UTC offset).
- **Settings persistence** — load/save all persisted fields, apply
 timezone changes (name → POSIX string → configure the library's TZ,
 making daylight saving automatic where applicable), generate/regenerate
 the API key, persist/connect with WiFi credentials, persist each
 rotation slot's settings.
- **External data fetching** — checks, on every loop cycle, whether the
 Weather and Quotes refresh timers have fired; if so, performs the HTTP
 request with a short timeout, parses the JSON response, updates the
 slot's global cache, and signals new-data availability to the display
 layer. Never blocks the loop — if the request exceeds the timeout, it is
 discarded and a new attempt is scheduled for the next interval.
- **Web interface page** — a single self-contained HTML/CSS/JS page (no
 external assets, no build step), served on one route. Organized into
 tabs: **Clock** (timezone, language, NTP, date interval, brightness,
 scroll); **Weather** (enable/disable slot, coordinates, intervals,
 unit); **Quotes** (enable/disable slot, ticker list, intervals);
 **Network** (current connection info, WiFi credentials form); **API**
 (auth toggle, key display/regeneration). The Clock tab includes a live
 preview of the display with the current time.
- **Web/API routes** — an asynchronous HTTP server registering:
 - `GET /` — the interface page.
 - `GET /api/display` — the single REST endpoint that reads query
   parameters to set the message text, scroll speed, brightness, display
   submode, main mode, timezone, date on/off, date interval, and a single
   "alert" message. It is the only route protected by the optional API
   key.
 - A pair of settings routes (`GET`/`POST`) for the web interface to
   read/write the same parameters, sharing validation logic with the
   REST endpoint instead of duplicating it.
 - Small JSON endpoints for: network/connection info, API auth status,
   key regeneration, and a list of timezone names to populate the
   interface's dropdown.
 - Build every JSON response with a structured JSON library/builder —
   never concatenate JSON strings manually.

## 6. Runtime behavior to reproduce

- **Boot sequence**: check the factory-reset button; if pressed, wipe all
 persisted settings. Otherwise, load saved settings and try to connect to
 the saved WiFi network with a timeout; on failure, fall back to an open
 configuration Access Point (fixed SSID, e.g., `<Product>-Setup`) with a
 fixed local IP, so the user can provide new credentials from a
 phone/laptop without needing the original network. Saving new
 credentials in that AP schedules a deferred restart for the device to
 reconnect as a station.
- **Main loop**: check WiFi/HTTP server; check the "alert message pending"
 flag (set by the HTTP layer, queued by the display layer); check the
 external-data-fetch timers (Weather and Quotes) and fire HTTP requests
 when needed; advance the slot rotation manager (active-slot switch
 timer); advance the current active slot (clock scroll/blink timer +
 periodic date-screen timer + NTP re-sync timer) — each with its own
 independent, non-blocking interval.

## 7. Suggested dependencies

- An Arduino-compatible core for the target microcontroller.
- A MAX72XX-family LED matrix driver library (or similar) with a
 text/column API via shift register, with scroll support.
- An asynchronous HTTP server library (avoid synchronous/blocking server
 libraries — they will freeze the display).
- A JSON library to build API responses and parse responses from external
 APIs (Open-Meteo, Yahoo Finance).
- The platform's key-value persistence API (NVS on ESP32).
- **HTTP client for external APIs** (Arduino ESP32's `HTTPClient` or
 equivalent) — used to query Open-Meteo and Yahoo Finance. Caution: the
 standard `HTTPClient` is synchronous and blocks the loop during the
 request. Mandatory mitigation: call it only from `loop()` when a timer
 fires (never inside HTTP handlers), with a short timeout (e.g., 3-5 s)
 and outside the critical rendering path.

## 8. Suggested additional documentation

Once the firmware exists, document (in files separate from this one):

- REST API reference: every query parameter, type/range/allowed list,
 auth header, `curl` examples, and JSON response formats.
- Wiring/hardware table (module pin → microcontroller pin).
- Third-party integration examples (e.g., a home automation platform
 calling the REST endpoint in an automation).

## 9. Out of scope for a v1

Keep the first implementation lean — these are natural evolutions, not
day-one requirements:

The v1 slots are: **Clock** (base mode), **Alert Message**, **Weather**
(Open-Meteo), and **Quotes** (Yahoo Finance). The following is explicitly
out of scope for the first version:

- Additional display slots beyond the four above.
- Multiple queued alerts (v1 keeps only the most recent).
- Any authentication beyond a single shared API key on the programmatic
 endpoint.
- OTA firmware updates or browser-based flashing (can be added later).
- Time-of-day slot scheduling (e.g., showing quotes only during market
 hours).
