# Smart Matrix Clock — User How-To Guide

This guide walks you through every user-visible feature of the Smart Matrix Clock, from first power-on to day-to-day operation. No programming or command-line knowledge is required for normal use — everything described here can be done through the web panel in your browser.

---

## Table of Contents

1. [First power-on](#1-first-power-on)
2. [Connecting to your WiFi network](#2-connecting-to-your-wifi-network)
3. [Finding the device on your network](#3-finding-the-device-on-your-network)
4. [The web panel overview](#4-the-web-panel-overview)
5. [Clock tab — time, date and display](#5-clock-tab--time-date-and-display)
6. [Weather tab](#6-weather-tab)
7. [Quotes tab](#7-quotes-tab)
8. [WiFi tab](#8-wifi-tab)
9. [Sending a message](#9-sending-a-message)
10. [Message history](#10-message-history)
11. [Factory reset](#11-factory-reset)
12. [Automation with curl / REST API](#12-automation-with-curl--rest-api)
13. [Home Assistant integration](#13-home-assistant-integration)
14. [Troubleshooting](#14-troubleshooting)

---

## 1. First power-on

When the clock is powered for the first time (or after a factory reset), it has no WiFi credentials stored. It will:

1. Show `----` or `--:--` on the display while starting up.
2. Open a WiFi Access Point named **`SmartMatrixClock-Setup`** (no password required).
3. Wait for you to connect and configure it.

The display shows the fallback clock face (`--:--`) until NTP time is successfully fetched after WiFi is connected.

---

## 2. Connecting to your WiFi network

### Step-by-step

1. On your phone or computer, open the WiFi settings and connect to the network **`SmartMatrixClock-Setup`**.
   - No password is required.
2. Open a web browser and go to **`http://192.168.4.1`**.
3. The web panel will open. Click the **WiFi** tab.
4. Click **Scan networks** to see the available networks around you.
5. Click on your network name in the list — the SSID field will be filled automatically.
6. Enter your network password in the **Password** field.
7. Click **Save & Reboot**.

The device will save the credentials and reboot. After ~5–10 seconds it will connect to your network, fetch the time from NTP, and start the clock display.

> ⚠️ If the device cannot connect (wrong password, network out of range), it will fall back to AP mode again and open the `SmartMatrixClock-Setup` hotspot so you can try again.

### Open (passwordless) networks

Leave the Password field empty and click **Save & Reboot**.

---

## 3. Finding the device on your network

After the clock connects to your router, it gets a local IP address. There are a few ways to find it:

- **Web panel title bar** — the IP is shown in the header of the web panel once the device is connected.
- **Router admin page** — look for `SmartMatrixClock` in the DHCP client list of your router.
- **Serial monitor** — if connected via USB, open a serial monitor at 115200 baud; the IP is printed on startup.
- **mDNS / Bonjour** — not yet implemented; use the IP directly.

Once you know the IP (e.g. `192.168.1.42`), open **`http://192.168.1.42`** in any browser on the same network to reach the web panel.

---

## 4. The web panel overview

The web panel is a single-page app served directly by the ESP32. It has five tabs:

| Tab | Purpose |
|---|---|
| **Clock** | Time format, timezone, locale, brightness, date display, night mode |
| **Weather** | Enable/disable weather slot, set location, temperature unit, scheduling |
| **Quotes** | Enable/disable quotes slot, set ticker symbols, scheduling |
| **WiFi** | Change the WiFi network |
| **Message** | Send an ad-hoc message to the display |

The panel header shows the current time (updated every second), the connected WiFi network, the device IP, and the firmware version.

### Language

The web panel supports **English** and **Portuguese**. A language switcher button (EN / PT) is available at the top right of the panel. Your choice is stored in the browser — not on the device.

---

## 5. Clock tab — time, date and display

### Brightness

The **Brightness** slider (0–15) controls the LED intensity. Use lower values at night or in dark rooms. Changes take effect immediately.

### Scroll speed

The **Scroll speed** slider controls how fast scrolling text moves (date, weather, quotes, messages). The value is in milliseconds per frame — **lower = faster**.

### Clock mode

Two modes are available:

| Mode | Display |
|---|---|
| **HH:MM** | Hour and minutes with a blinking colon. Default. |
| **HH:MM:SS** | Hour, minutes, and seconds. Updates every second. |

Check the **Show seconds (HH:MM:SS)** checkbox to switch to seconds mode.

### Timezone

Select your timezone from the dropdown. The full IANA timezone list is available. Changes apply immediately — no reboot required.

### Locale

Controls two things:
- **Date display** — day and month names are shown in the selected language (English or Portuguese).
- **Weather** — the city/postal-code search (Geocoding) uses the selected language for place names.

### Date display

Enable the **Show date periodically** checkbox to have the date scroll across the display at a configurable interval. The **Interval** field sets how often (in seconds) the date appears between clock ticks.

### Night mode (auto dim)

Night mode automatically lowers the display brightness during a configured time window.

1. Check **Night mode (auto dim)** to enable it.
2. Set the **Night brightness** level (0 = completely off is valid).
3. Set the **Night starts at** and **Night ends at** times.
   - The window can cross midnight (e.g. 23:00 → 07:00).
   - Equal start and end times disable the window (the feature remains enabled but is effectively always-on at normal brightness).
4. Click **Save**.

The normal brightness configured above resumes automatically when the night window ends.

---

## 6. Weather tab

The weather slot fetches current temperature, condition, and daily min/max from [Open-Meteo](https://open-meteo.com/) (free, no API key required).

### Enabling weather

1. Toggle **Weather slot enabled** on.
2. Set your location (see below).
3. Click **Save**.

### Setting your location

**Option A — City / postal-code search (easiest):**
1. Type a city name or postal code in the **Search city or postal code** field.
2. A list of matching places will appear. Click on your location.
3. The latitude and longitude fields are filled automatically.
4. Click **Save**.

**Option B — Manual coordinates:**
1. Enter your latitude and longitude directly in the **Latitude** and **Longitude** fields.
2. Click **Save**.

### Temperature unit

Select **°C** (Celsius) or **°F** (Fahrenheit).

### Update interval

How often (in minutes or hours) the device fetches fresh weather data from Open-Meteo. The minimum is 5 minutes.

### Display duration

How long the weather slot stays on screen during each rotation cycle.

### Scheduling

You can restrict the weather slot to specific days and times:

- **Days** — select which weekdays the slot is active (all days are selected by default).
- **Start / End time** — set a daily time window. Leave them equal (both `00:00`) for "always active".
  - The window can cross midnight (e.g. 08:00 → 20:00 means the slot is only visible from 8 in the morning to 8 in the evening).

### Preview on display

Click **Preview on display** to force the weather slot to appear on the LED matrix immediately, without waiting for the rotation timer.

### Force refresh

Click **Force refresh** to trigger an immediate re-fetch of weather data from Open-Meteo, bypassing the configured update interval.

### Stale data indicator

If the last fetch failed but cached data exists, the display will prefix the weather text with `*` to indicate the data may be outdated.

---

## 7. Quotes tab

The quotes slot displays price and percentage change for configurable financial assets (stocks, crypto, indices) via Yahoo Finance.

### Enabling quotes

1. Toggle **Quotes slot enabled** on.
2. Enter one or more ticker symbols (see below).
3. Click **Save**.

### Ticker symbols

Enter ticker symbols in the **Tickers** field, separated by commas. Examples:

```
AAPL,MSFT,GOOG
BTC-USD,ETH-USD
^GSPC,^IXIC
PETR4.SA,VALE3.SA
```

Yahoo Finance ticker symbols are used — the same ones you would search on [finance.yahoo.com](https://finance.yahoo.com).

### Display duration

How long the quotes slot stays on screen per rotation.

### Scheduling

Restrict the quotes slot to specific days and times — useful for showing market data only during trading hours:

- **Default schedule**: Monday–Friday, 08:00–18:00.
- Use the **Days** checkboxes and **Start / End time** fields to change the window.
- Leave Start and End both at `00:00` for "always active".

### Preview on display

Click **Preview on display** to force the quotes slot on screen immediately.

### Force refresh

Click **Force refresh** to re-fetch quotes data immediately.

---

## 8. WiFi tab

### Changing your network

1. Click **Scan networks** to see available networks.
2. Click on your network in the list to fill the **SSID** field.
3. Enter the **Password**.
4. Click **Save & Reboot**.

> ⚠️ The device reboots immediately after saving. You will lose the current browser connection. Wait ~10 seconds and reconnect to the device's new IP on the new network.

---

## 9. Sending a message

Messages interrupt the current slot rotation and appear immediately on the display. After the message finishes, the clock resumes normal rotation.

### Via the web panel

1. Open the **Message** tab (or find the message section on the main panel).
2. Type your message text in the **Message** field.
3. Choose a **Display mode**:
   - **Scroll** — text scrolls right to left (good for long messages).
   - **Blink** — text blinks on and off (good for alerts).
   - **Static** — text stays still (good for short messages).
   - **Blink + Scroll** — blinks first, then scrolls; repeats for the total duration.
4. Optionally set a **Duration** (Blink, Static, and Blink+Scroll modes).
5. Optionally override **Brightness** and **Scroll speed** just for this message.
6. Click **Send**.

A live preview is shown on screen before you send, so you can see how the message will look on the display.

### Icons in messages

You can embed special icons into your message text using `[tag]` notation:

| Tag | Description |
|---|---|
| `[heart]` | Heart symbol ♥ |
| `[diamond]` | Diamond ♦ |
| `[spade]` | Spade ♠ |
| `[bullet]` | Bullet point • |
| `[star]` | Snowflake/star glyph |
| `[arrow_right]` | Right-pointing triangle ▶ |
| `[arrow_left]` | Left-pointing triangle ◀ |
| `[up]` | Up arrow ↑ |
| `[down]` | Down arrow ↓ |
| `[bell]` | Solid up-triangle (attention) ▲ |
| `[warn]` | Solid down-triangle (warning) ▼ |

**Example:** `Warning [warn] High temperature!` will show the warn glyph between the words.

> **Tip:** The display has 32 LED columns (roughly 5–6 characters visible at once with the default font). For longer messages, use **Scroll** or **Blink + Scroll** mode to ensure the full text is readable.

---

## 10. Message history

The device keeps the last 20 messages in memory (resets on reboot). To view them:

- In the web panel, look for the **History** section under the Message tab.
- Via the REST API: `GET http://<ip>/api/messages/history`

Each entry includes the message text and a timestamp (Unix time when the message was received).

---

## 11. Factory reset

A factory reset wipes all stored settings (WiFi credentials, timezone, brightness, weather/quotes configuration) and returns the device to its out-of-box state.

### How to perform a factory reset

1. **Power off** the device.
2. **Hold the BOOT button** (GPIO 0) on the ESP32 board.
3. **Power on** while keeping the button held.
4. Release the button after ~2–3 seconds.

The device will erase all NVS data and reboot into **AP mode** (`SmartMatrixClock-Setup`), ready to be reconfigured.

> ⚠️ This operation is irreversible. All configuration will be lost.

---

## 12. Automation with curl / REST API

Every feature available in the web panel is also accessible via a simple HTTP REST API. This is useful for:

- Sending notifications from scripts or CI/CD pipelines.
- Integrating with home automation platforms.
- Changing settings programmatically.

**Base URL:** `http://<DEVICE_IP>` (port 80)

### Send a message

```bash
curl -X POST http://192.168.1.42/api/message \
  -H "Content-Type: application/json" \
  -d '{"message": "Build passed!", "mode": 0}'
```

### Send a blinking alert for 10 seconds

```bash
curl -X POST http://192.168.1.42/api/message \
  -H "Content-Type: application/json" \
  -d '{"message": "DOORBELL", "mode": 1, "duration_ms": 10000, "brightness": 15}'
```

### Change brightness

```bash
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"brightness": 3}'
```

### Enable weather slot and set coordinates

```bash
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"weather_enabled": true, "weather_lat": -23.55, "weather_lon": -46.63}'
```

### Check device status

```bash
curl http://192.168.1.42/api/status
```

Returns current time, NTP sync status, active slot, WiFi RSSI, memory usage, and cache state.

### Full API reference

See [`docs/api-rest.md`](api-rest.md) for the complete endpoint documentation including all fields, ranges, and error codes.

---

## 13. Home Assistant integration

The Smart Matrix Clock integrates with Home Assistant using the built-in `rest_command` integration — no custom component is required.

Key use cases:
- Send notifications from Home Assistant automations (doorbell, door sensor, motion, etc.)
- Monitor device status as a sensor
- Control brightness from the dashboard

See the dedicated guide: [`docs/home-assistant.md`](home-assistant.md)

---

## 14. Troubleshooting

### Display shows `--:--` permanently

The device has not synced time via NTP. Possible causes:

- WiFi not connected — check the web panel header for the connected network.
- NTP server unreachable — try changing the NTP server in the Clock tab (e.g. `time.google.com`).
- Incorrect timezone — verify the timezone is set correctly in the Clock tab.

### Display shows nothing

- Check that the device is powered (USB or 5V supply).
- Brightness may be set to 0 — open the web panel and raise the brightness slider.
- Night mode may have set brightness to 0 — check the Night mode settings in the Clock tab.

### Web panel is unreachable

- Confirm the device is connected to your network (the display will show the clock, not `----`).
- Check your router's DHCP list for the device's current IP.
- Try accessing `http://192.168.4.1` — if you see the panel, the device fell back to AP mode.

### Weather slot not appearing

- Ensure **Weather slot enabled** is toggled on in the Weather tab.
- Verify the coordinates are set (Latitude / Longitude fields must not both be `0.0` if you are not near the equator/prime meridian).
- Check the **Scheduling** settings — the slot may be configured to appear only on certain days or times.
- Look at `GET /api/status` for `weather_cache_valid` and `weather_cache_stale` values.

### Quotes slot not appearing

- Ensure **Quotes slot enabled** is on and at least one ticker symbol is entered.
- Verify the scheduling window includes the current day and time (default: Mon–Fri 08:00–18:00).
- Yahoo Finance requires a valid ticker symbol — verify your symbols at [finance.yahoo.com](https://finance.yahoo.com).

### Message not appearing on the display

- Only the **most recent** message is stored. If two messages are sent in quick succession, only the second is shown.
- Check that the message text is not empty.
- The mode `1` (Blink) and mode `2` (Static) require a `duration_ms` value — omitting it uses the default.

### Icons not showing correctly

- Use the exact tag names listed in the [Icons in messages](#icons-in-messages) section.
- Tags are case-sensitive: `[warn]` works; `[WARN]` does not.
- Unknown tags are displayed as literal text (e.g. `[unknown]`).

### Device not connecting after saving WiFi credentials

- Double-check the password — it is case-sensitive.
- Move the device closer to the router during initial setup if the signal is weak.
- If the device keeps falling back to AP mode, it cannot reach the network — check that the SSID is exactly correct (no extra spaces).

### How to restore defaults without factory reset

You can reset individual settings via the web panel or REST API without losing all configuration. For example, to restore the default NTP server:

```bash
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"ntp_server": "pool.ntp.org"}'
```

For a complete reset see [Section 11 — Factory reset](#11-factory-reset).
