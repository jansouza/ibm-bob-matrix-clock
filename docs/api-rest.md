# Smart Matrix Clock — REST API Reference

All endpoints are served directly by the ESP32 on port **80**.  
Base URL: `http://<DEVICE_IP>`

---

## General conventions

| Item | Detail |
|---|---|
| Format | JSON (`Content-Type: application/json`) |
| Charset | UTF-8 in the request body; the firmware converts it to Latin-1 internally before rendering |
| Success response | HTTP 200 with `{"ok": true}` (or an object with additional fields) |
| Error response | HTTP 4xx with `{"ok": false, "error": "<message>"}` |
| Persistence | Every configuration change is written to flash (NVS) and survives reboots |
| Immediate effect | Saving a configuration value applies it in real time — no reboot required (except WiFi credentials) |

---

## Authentication

When API key authentication is enabled, **every endpoint** requires the following header:

```
X-API-Key: <32-hex-character key>
```

The key is managed from the **API** tab in the web interface.  
When authentication is disabled (default), all endpoints are open.

> Any request missing or providing an invalid key returns **401 Unauthorized** while authentication is enabled.

---

## Endpoints

### `GET /`

Returns the built-in web interface (self-contained HTML/CSS/JS).

**Response:** `200 text/html`

---

### `GET /api/status`

Returns the current operational state of the device.

**Response `200`:**

```json
{
  "ntp_synced":  true,
  "active_slot": 0,
  "ssid":        "MyNetwork",
  "ip":          "192.168.1.42",
  "time_str":    "14:35"
}
```

| Field | Type | Description |
|---|---|---|
| `ntp_synced` | bool | `true` after the first successful NTP synchronisation |
| `active_slot` | int | Index of the currently active slot (0=clock, 1=alert, 2=weather, 3=quotes) |
| `ssid` | string | SSID of the connected WiFi network; `""` when in AP mode |
| `ip` | string | IP address of the active interface (station or AP softIP) |
| `time_str` | string | Current time as `HH:MM`; `"--:--"` if NTP has not synced yet |

---

### `GET /api/config`

Returns the full persisted configuration.

**Response `200`:**

```json
{
  "brightness":         4,
  "scroll_speed_ms":    50,
  "timezone":           "America/Sao_Paulo",
  "language":           "pt",
  "ntp_server":         "pool.ntp.org",
  "date_interval_ms":   30000,
  "date_enabled":       true,
  "weather_enabled":    false,
  "weather_update_ms":  30000,
  "weather_display_ms": 30000,
  "weather_lat":        0.0,
  "weather_lon":        0.0,
  "temp_unit":          "C",
  "quotes_enabled":     false,
  "quotes_update_ms":   30000,
  "quotes_display_ms":  30000,
  "quotes_tickers":     ""
}
```

> **Note:** the `api_key` field is intentionally omitted from this response for security. The key is only visible in the web interface (API tab).

---

### `POST /api/config`

Updates one or more configuration fields. Only fields present in the body are processed — absent fields are left unchanged.

**Content-Type:** `application/json`

#### Accepted fields

##### Display

| Field | Type | Range | Description |
|---|---|---|---|
| `brightness` | int | 0–15 | Display intensity (0 = minimum, 15 = maximum) |
| `scroll_speed_ms` | int | 10–200 | Scroll speed in ms per frame (10 = fastest, 200 = slowest) |

##### Clock / NTP

| Field | Type | Range | Description |
|---|---|---|---|
| `timezone` | string | — | IANA timezone name (e.g. `"America/Sao_Paulo"`). Must be a valid entry from `GET /api/timezones` |
| `language` | string | `"pt"` or `"en"` | Language for weekday names shown on the date screen |
| `ntp_server` | string | 1–63 chars | NTP server hostname (default: `"pool.ntp.org"`) |
| `date_enabled` | bool | — | `true` to show the date periodically |
| `date_interval_ms` | int | 5000–300000 | Interval between date appearances (ms) |

##### Slots

| Field | Type | Range | Description |
|---|---|---|---|
| `weather_enabled` | bool | — | Enable/disable the weather slot (slot 2) |
| `weather_display_ms` | int | 5000–300000 | Weather slot display duration (ms) |
| `quotes_enabled` | bool | — | Enable/disable the quotes slot (slot 3) |
| `quotes_display_ms` | int | 5000–300000 | Quotes slot display duration (ms) |

#### Normal response `200`:

```json
{ "ok": true }
```

#### Examples

```bash
# Change brightness and scroll speed
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"brightness": 8, "scroll_speed_ms": 40}'

# Change timezone
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"timezone": "America/Fortaleza"}'

# Enable periodic date display every 60 s
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -d '{"date_enabled": true, "date_interval_ms": 60000}'

# With authentication enabled
curl -X POST http://192.168.1.42/api/config \
  -H "Content-Type: application/json" \
  -H "X-API-Key: a3f8c21d9e054b67abcdef1234567890" \
  -d '{"brightness": 5}'
```

#### Possible errors

| Code | Message | Cause |
|---|---|---|
| 400 | `"Invalid JSON"` | Request body is not valid JSON |
| 400 | `"brightness must be 0-15"` | Value out of range |
| 400 | `"scroll_speed_ms must be 10-200"` | Value out of range |
| 400 | `"Unknown timezone"` | IANA name not found in the internal table |
| 400 | `"language must be 'pt' or 'en'"` | Unrecognised value |
| 400 | `"ntp_server invalid length"` | Empty string or longer than 63 chars |
| 400 | `"date_interval_ms out of range"` | Value outside 5000–300000 ms |
| 400 | `"weather_display_ms out of range"` | Value outside 5000–300000 ms |
| 400 | `"quotes_display_ms out of range"` | Value outside 5000–300000 ms |
| 401 | `"Unauthorized: invalid or missing API key"` | Key missing or invalid (when auth is enabled) |

---

### `POST /api/alert`

Sends an alert message to be displayed immediately on the LED matrix.  
The alert has **top priority**: it interrupts the current slot and is shown next.  
It is **one-shot**: only the most recent alert is stored.

**Content-Type:** `application/json`

#### Body

```json
{
  "message":     "Alert text",
  "mode":        0,
  "duration_ms": 5000
}
```

| Field | Type | Required | Range | Description |
|---|---|---|---|---|
| `message` | string | **yes** | 1–127 chars | Text to display. Accepts UTF-8 (accents, special characters). Max 127 useful characters. |
| `mode` | int | no | 0–3 | Display mode (see table below). Default: previously configured value (default `0`) |
| `duration_ms` | int | no | 1000–60000 | Duration in ms for Blink, Static, and Blink+Scroll modes. Ignored in pure Scroll mode. |

#### Display modes (`mode`)

| Value | Name | Behaviour |
|---|---|---|
| `0` | **Scroll** | Text scrolls from right to left. `duration_ms` limits total scroll time; if reached before the text finishes, the scroll is cut off. |
| `1` | **Blink** | Text is displayed centred and blinks on/off every 500 ms for `duration_ms`. |
| `2` | **Static** | Text is displayed centred and still for `duration_ms`. |
| `3` | **Blink + Scroll** | **Phase 1:** text blinks on screen for `duration_ms`. **Phase 2:** after blinking, the full text scrolls once from right to left. Useful for grabbing attention first, then showing a long message. |

> **Note on long text in Blink / Static / Blink+Scroll Phase 1:** the display has 32 LED columns (4 modules × 8 columns). With the default font, approximately 5–6 characters fit at once. Longer texts are truncated in static/blink display; use Scroll or Blink+Scroll to ensure the full message is readable.

#### Response `200`:

```json
{ "ok": true }
```

#### Examples

```bash
# Simple scrolling alert (no auth)
curl -X POST http://192.168.1.42/api/alert \
  -H "Content-Type: application/json" \
  -d '{"message": "Door open!"}'

# Blinking alert for 8 seconds
curl -X POST http://192.168.1.42/api/alert \
  -H "Content-Type: application/json" \
  -d '{"message": "ALERT", "mode": 1, "duration_ms": 8000}'

# Static alert for 10 seconds
curl -X POST http://192.168.1.42/api/alert \
  -H "Content-Type: application/json" \
  -d '{"message": "MEETING", "mode": 2, "duration_ms": 10000}'

# Blink 5 s then scroll — long message
curl -X POST http://192.168.1.42/api/alert \
  -H "Content-Type: application/json" \
  -d '{"message": "Critical temperature on server A3!", "mode": 3, "duration_ms": 5000}'

# With authentication enabled
curl -X POST http://192.168.1.42/api/alert \
  -H "Content-Type: application/json" \
  -H "X-API-Key: a3f8c21d9e054b67abcdef1234567890" \
  -d '{"message": "Deploy done", "mode": 0}'
```

#### Possible errors

| Code | Message | Cause |
|---|---|---|
| 400 | `"Invalid JSON"` | Request body is not valid JSON |
| 400 | `"Missing 'message' field"` | `message` field absent |
| 400 | `"message is empty"` | `message` field is an empty string |
| 400 | `"mode must be 0-3"` | `mode` value out of range |
| 400 | `"duration_ms must be 1000-60000"` | `duration_ms` value out of range |
| 401 | `"Unauthorized: invalid or missing API key"` | Header `X-API-Key` missing or invalid (when auth is enabled) |

---

### `GET /api/timezones`

Returns the full list of IANA timezones supported by the firmware.

**Response `200`:**

```json
[
  "Africa/Abidjan",
  "America/Sao_Paulo",
  "America/Fortaleza",
  "America/Manaus",
  "America/Noronha",
  "Europe/Lisbon",
  "Europe/London",
  "..."
]
```

Use the values from this list in the `timezone` field of `POST /api/config`.

---

### `POST /api/wifi`

Saves new WiFi credentials and reboots the device to connect to the new network.

> ⚠️ **This call triggers an immediate reboot (~1.5 s).** The connection to the device will be interrupted. After rebooting, the device will attempt to connect to the new network. If it fails, it will fall back to AP mode (`SmartMatrixClock-Setup` / `192.168.4.1`).

**Content-Type:** `application/json`

#### Body

```json
{
  "ssid":     "NetworkName",
  "password": "NetworkPassword"
}
```

| Field | Type | Required | Range | Description |
|---|---|---|---|---|
| `ssid` | string | **yes** | 1–32 chars | WiFi network name |
| `password` | string | no | 0–64 chars | Network password. Omit or use `""` for open networks |

#### Response `200`:

```json
{ "ok": true }
```

#### Examples

```bash
# Connect to a password-protected network
curl -X POST http://192.168.1.42/api/wifi \
  -H "Content-Type: application/json" \
  -d '{"ssid": "MyNetwork", "password": "my-password"}'

# Connect to an open network
curl -X POST http://192.168.1.42/api/wifi \
  -H "Content-Type: application/json" \
  -d '{"ssid": "OpenNetwork"}'

# With authentication enabled
curl -X POST http://192.168.1.42/api/wifi \
  -H "Content-Type: application/json" \
  -H "X-API-Key: a3f8c21d9e054b67abcdef1234567890" \
  -d '{"ssid": "MyNetwork", "password": "my-password"}'
```

#### Possible errors

| Code | Message | Cause |
|---|---|---|
| 400 | `"Invalid JSON"` | Request body is not valid JSON |
| 400 | `"Missing 'ssid' field"` | `ssid` field absent |
| 400 | `"ssid invalid length"` | Empty SSID or longer than 32 chars |
| 400 | `"password too long"` | Password longer than 64 chars |
| 401 | `"Unauthorized: invalid or missing API key"` | Key missing or invalid (when auth is enabled) |

---

## General error behaviour

Any unmapped route returns:

```json
{ "ok": false, "error": "Not found" }
```

with HTTP status **404**.

---

## AP mode (initial setup)

If the device has no saved WiFi credentials, or if the connection fails, it enters **Access Point mode**:

| Item | Value |
|---|---|
| SSID | `SmartMatrixClock-Setup` |
| Password | *(open, no password)* |
| IP | `192.168.4.1` |

Connect to this network and open `http://192.168.4.1` to configure WiFi via the web interface, or use `POST /api/wifi` directly.

---

## Endpoint summary

| Method | Endpoint | Auth required¹ | Description |
|---|---|---|---|
| GET | `/` | — | Web interface |
| GET | `/api/status` | — | Real-time operational state |
| GET | `/api/config` | — | Read persisted configuration |
| POST | `/api/config` | ✓ | Update configuration |
| POST | `/api/alert` | ✓ | Send an alert message to the display |
| GET | `/api/timezones` | — | List of supported timezones |
| POST | `/api/wifi` | ✓ | Save WiFi credentials and reboot |

¹ Authentication is required only when it is enabled via the web interface API tab. When disabled (default), all endpoints are open.
`GET /`, `GET /api/status`, `GET /api/config`, and `GET /api/timezones` are always open — they are used by the built-in web interface and carry no sensitive write operations.
