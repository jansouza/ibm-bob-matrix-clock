# AGENTS.md — Ask mode

This file provides guidance to agents answering questions about this repository.

## Non-Obvious Architecture

- **`loop()` is just three ticks**: `wifiTick()`, `ntpTick()`, `displayTick()` — all state machines, no blocking.
- **`web_page.cpp` is a single 66 KB raw-string literal** (`const char WEB_PAGE_HTML[] = R"=====(...)====="`). The entire HTML/CSS/JS UI lives in one string — no separate files. Served via `req->send(200, "text/html", WEB_PAGE_HTML)` in `web_routes.cpp`.
- **Two independent language settings**: `cfgLanguage` = on-device clock locale (day/month names via `locale_data.cpp`); `cfgUiLanguage` = web panel UI language. Completely separate NVS keys, separate config fields (`locale` vs `ui_language`).
- **`_uiLanguages[]` in `persistence.cpp`** is the only place where valid UI language codes are listed — `web_routes.cpp` calls `isUiLanguageValid()` which loops over that array.
- **Slot scheduling**: `start == end` means "no restriction / always active" — NOT an invalid range. Day mask uses `tm_wday` bit positions (0=Sunday).
- **Message history** is a ring buffer in `globals.h` (`messageHistory[]`, 20 entries). `messageHistoryHead` points to the oldest entry. Exposed read-only at `GET /api/messages/history`.
- **`_ssLayout(hmWidthPx)`** in `display.cpp` computes the seconds start column dynamically by measuring the `HH:MM` pixel width — the position is not fixed; it adapts to the font rendering.
- **`WEB_PAGE_HTML` is not in PROGMEM** — it's `const char[]`, which the ESP32 linker places in flash but accessible as a pointer without explicit PROGMEM macros. The string is ~66 KB and does not consume SRAM.
- **`pollStatus()` in the web panel runs every 1 second** — `time_str` in `/api/status` already reflects `HH:MM:SS` vs `HH:MM` based on `cfgClockMode`, so the live preview stays accurate without client-side mode-tracking.

## Counterintuitive File Locations

- `applyTimezone()` lives in `persistence.cpp`, not `ntp.cpp` — it's called both from `setup()` and from `ntpTick()` on re-sync.
- `expandIconTags()` lives in `text_encoding.cpp` alongside UTF-8 encoding, not in `display.cpp`.
- `factoryReset()` lives in `persistence.cpp` — it wipes NVS but the BOOT-button check that triggers it is in the main `.ino` file.
- `scheduleRestart()` lives in `wifi_manager.cpp` — it's used for all restarts, not just WiFi-related ones.
- Slot scheduling state (`slotScheduleStartMin[]` etc.) is in `globals.h/cpp`, checked inside `display.cpp`'s `slotRotationTick()`.

## What Does Not Exist

- No unit tests — validation is on physical hardware only.
- No authentication — a previous `X-API-Key` mechanism was removed because the panel's JS never sent the header.
- No OTA update endpoint yet — `POST /api/ota` is planned (Sub-Task 6c) but not implemented.
- No WiFi scan endpoint yet — `GET /api/wifi/scan` is planned (Sub-Task 5) but not implemented.
- No HTTP Basic Auth yet — planned (Sub-Task 3) but not implemented.
