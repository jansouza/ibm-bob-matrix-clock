# AGENTS.md — Ask mode

This file provides guidance to agents answering questions about this repository.

## Non-Obvious Architecture

- **`loop()` is four ticks only**: `wifiTick()`, `ntpTick()`, `fetcherTick()`, `displayTick()` — all state machines, no blocking.
- **`WEB_PAGE_HTML` is a monolithic ~62 KB raw-string literal** in `web_page.cpp`. The entire HTML/CSS/JS UI lives in one string — no separate files. Not PROGMEM — `const char[]` placed in flash by the linker, accessible as pointer without PROGMEM macros, does not consume SRAM.
- **`pollStatus()` in the web panel runs every 1 second** — `/api/status` `time_str` already reflects `HH:MM:SS` vs `HH:MM` based on `cfgClockMode`, so the live preview stays accurate without client-side mode tracking.
- **Two independent language settings** (commonly confused):
  - `cfgLocale` (NVS key `"locale"`) = on-device content locale: date names, number format, quote search lang
  - `cfgUiLanguage` (NVS key `"ui_lang"`) = web panel UI language
  - There is **no `cfgLanguage` variable** — older docs that say so are wrong.
- **Slot scheduling**: `start == end` means "no restriction / always active" — NOT an invalid range. Day mask uses `tm_wday` bits (0=Sunday).
- **`_ssLayout(hmWidthPx)`** computes the seconds start column dynamically by measuring `HH:MM` pixel width — position is not fixed.
- **Message history** is a 20-entry ring buffer in `globals.h`. `messageHistoryHead` = oldest entry. Exposed read-only at `GET /api/messages/history`.
- **Four message modes**: SCROLL (0), BLINK (1), STATIC (2), BLINK_SCROLL (3). `BLINK_SCROLL` cycles blink→scroll phases; `messageDurationMs` is the total cycle duration.
- **No authentication on any endpoint** — an `X-API-Key` mechanism was implemented then removed (browser JS never sent the header, locking the panel out).

## Counterintuitive File Locations

- `applyTimezone()` lives in `persistence.cpp`, not `ntp.cpp` — called both from `setup()` and from `ntpTick()` on re-sync.
- `expandIconTags()` lives in `text_encoding.cpp`, not `display.cpp`.
- `factoryReset()` lives in `persistence.cpp` — the BOOT-button check that triggers it is in `smart-matrix-clock-esp32.ino`.
- `scheduleRestart()` lives in `wifi_manager.cpp` — used for all restarts, not just WiFi-related ones.
- Slot scheduling state (`slotScheduleStartMin[]` etc.) is in `globals.h/cpp`, evaluated inside `display.cpp`'s `slotRotationTick()`.

## Pending Enhancements

See `docs/enhancements-plan.md`. Currently unimplemented: HTTP Basic Auth (ST-3), OTA update (ST-6c), soft reboot endpoint (ST-6f), 12-hour clock mode (ST-7), async WiFi scan (ST-9), MQTT push (ST-10), display test mode (ST-11), message queue/playlist (ST-12).

## Test Suite

Exists in `tests/` — host build, plain `g++`, no ESP32 required. Covers: `text_encoding.cpp`, `locale_data.cpp`, slot scheduling algorithm, ticker splitting, persistence language validation. Modules with Arduino/ESP32 deps are validated on-device only.
