# Smart Matrix Clock — Home Assistant Integration

This guide covers how to integrate the Smart Matrix Clock with [Home Assistant](https://www.home-assistant.io/) using the built-in `rest` and `rest_command` integrations — no custom component, HACS add-on, or Node-RED flow required.

All requests go directly to the ESP32 over HTTP on port **80**. No authentication is required on any endpoint.

---

## Contents

1. [Prerequisites](#1-prerequisites)
2. [Finding the device IP](#2-finding-the-device-ip)
3. [Sending messages from automations](#3-sending-messages-from-automations)
4. [Sensors — monitoring device status](#4-sensors--monitoring-device-status)
5. [Configuration via REST commands](#5-configuration-via-rest-commands)
6. [Practical automation examples](#6-practical-automation-examples)
7. [Dashboard card examples](#7-dashboard-card-examples)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Prerequisites

- Smart Matrix Clock connected to the same network as Home Assistant
- Home Assistant 2023.1 or later (uses `action:` syntax in automations)
- The device IP address (see section 2 below)

> **Tip:** assign the clock a **static IP** (DHCP reservation on your router or using the ESP32's hostname `SmartMatrixClock`) so the configuration never breaks when the router renews leases.

---

## 2. Finding the device IP

Navigate to the web panel at `http://smartmatrixclock.local` (mDNS, if your network supports it) or check your router's DHCP table.  
You can also call `GET /api/status` directly:

```bash
curl http://<device-ip>/api/status
```

Response:

```json
{
  "ntp_synced":  true,
  "active_slot": 0,
  "ssid":        "MyNetwork",
  "ip":          "192.168.1.42",
  "time_str":    "14:35"
}
```

Replace `192.168.1.42` throughout this guide with your actual device IP.

---

## 3. Sending messages from automations

### 3.1 Add `rest_command` entries to `configuration.yaml`

`rest_command` lets you call the clock's `POST /api/message` endpoint from any automation or script.

```yaml
# configuration.yaml

rest_command:
  matrix_clock_message:
    url: "http://192.168.1.42/api/message"
    method: POST
    headers:
      Content-Type: application/json
    payload: >-
      {
        "message": "{{ message }}",
        "mode": {{ mode | default(0) }},
        "brightness": {{ brightness | default(8) }},
        "scroll_speed_ms": {{ scroll_speed_ms | default(50) }}
      }
    content_type: "application/json"

  matrix_clock_message_blink:
    url: "http://192.168.1.42/api/message"
    method: POST
    headers:
      Content-Type: application/json
    payload: >-
      {
        "message": "{{ message }}",
        "mode": 1,
        "duration_ms": {{ duration_ms | default(5000) }},
        "brightness": 15
      }
    content_type: "application/json"
```

> **Reload** Home Assistant's `rest_command` configuration (`Developer Tools → YAML → REST Commands`) after saving.

### 3.2 Display modes reference

| `mode` | Behaviour |
|---|---|
| `0` | Scroll (default) — scrolls from right to left |
| `1` | Blink — centred text blinks for `duration_ms` ms |
| `2` | Static — centred text shown for `duration_ms` ms |
| `3` | Blink + Scroll — blinks 5 s then scrolls, cycle repeats |

### 3.3 Available icon tags

Embed these tags inside `message` for built-in LED glyphs:

| Tag | Displays as |
|---|---|
| `[heart]` | ♥ |
| `[bell]` | ▲ (attention triangle) |
| `[warn]` | ▼ (warning indicator) |
| `[star]` | ❄ |
| `[bullet]` | • |
| `[arrow_right]` | ▶ |
| `[arrow_left]` | ◀ |
| `[up]` | ↑ |
| `[down]` | ↓ |

Example: `"[warn] ALERT [warn]"` will render the warn glyph on both sides of the word ALERT.

---

## 4. Sensors — monitoring device status

### 4.1 Status sensor

Add a `rest` sensor to track whether the device is online and NTP-synced:

```yaml
# configuration.yaml

sensor:
  - platform: rest
    name: "Matrix Clock Status"
    unique_id: matrix_clock_status
    resource: "http://192.168.1.42/api/status"
    method: GET
    scan_interval: 30
    value_template: "{{ value_json.time_str }}"
    json_attributes:
      - ntp_synced
      - active_slot
      - ssid
      - ip
      - time_str
```

This creates a sensor `sensor.matrix_clock_status` whose:
- **state** = the current time string shown on the display (`HH:MM` or `HH:MM:SS`)
- **attributes** = the full `/api/status` response fields

### 4.2 Using the sensor in templates

```yaml
# In a template sensor or automation condition:

# Is the clock online and synced?
{{ states('sensor.matrix_clock_status') not in ['unavailable', 'unknown'] and
   state_attr('sensor.matrix_clock_status', 'ntp_synced') == true }}

# Which slot is currently active?
{{ state_attr('sensor.matrix_clock_status', 'active_slot') }}
# 0 = clock, 1 = message, 2 = weather, 3 = quotes

# Device IP
{{ state_attr('sensor.matrix_clock_status', 'ip') }}
```

### 4.3 Binary sensor (online/offline)

```yaml
# configuration.yaml

binary_sensor:
  - platform: template
    sensors:
      matrix_clock_online:
        friendly_name: "Matrix Clock Online"
        value_template: >-
          {{ states('sensor.matrix_clock_status') not in ['unavailable', 'unknown'] }}
        device_class: connectivity
```

---

## 5. Configuration via REST commands

### 5.1 Brightness control

```yaml
# configuration.yaml

rest_command:
  matrix_clock_set_brightness:
    url: "http://192.168.1.42/api/config"
    method: POST
    headers:
      Content-Type: application/json
    payload: '{"brightness": {{ brightness }} }'
    content_type: "application/json"
```

Use it in an automation:

```yaml
action:
  - action: rest_command.matrix_clock_set_brightness
    data:
      brightness: 2   # 0–15
```

### 5.2 Number helper + automation to control brightness from the dashboard

1. Create a **Number** helper in Home Assistant (Settings → Devices & Services → Helpers → Number):
   - Name: `Matrix Clock Brightness`
   - Entity ID: `input_number.matrix_clock_brightness`
   - Min: `0`, Max: `15`, Step: `1`

2. Add an automation that calls the REST command when the helper changes:

```yaml
automation:
  - alias: "Matrix Clock — Sync Brightness"
    trigger:
      - platform: state
        entity_id: input_number.matrix_clock_brightness
    action:
      - action: rest_command.matrix_clock_set_brightness
        data:
          brightness: "{{ states('input_number.matrix_clock_brightness') | int }}"
```

### 5.3 All configurable fields

Any field from `POST /api/config` can be set the same way. Here are the most useful ones for automations:

| Field | Type | Range | Description |
|---|---|---|---|
| `brightness` | int | 0–15 | Display brightness |
| `scroll_speed_ms` | int | 10–200 | Scroll speed (ms per frame) |
| `date_enabled` | bool | — | Show periodic date |
| `weather_enabled` | bool | — | Enable weather slot |
| `quotes_enabled` | bool | — | Enable quotes slot |
| `timezone` | string | — | IANA timezone |

Full field reference: [`docs/api-rest.md`](api-rest.md).

---

## 6. Practical automation examples

### 6.1 Notify when someone rings the doorbell

```yaml
automation:
  - alias: "Doorbell → Matrix Clock"
    trigger:
      - platform: state
        entity_id: binary_sensor.doorbell
        to: "on"
    action:
      - action: rest_command.matrix_clock_message_blink
        data:
          message: "[bell] DOORBELL [bell]"
          duration_ms: 8000
```

### 6.2 Alert when a door or window is left open

```yaml
automation:
  - alias: "Front Door Open Too Long → Matrix Clock"
    trigger:
      - platform: state
        entity_id: binary_sensor.front_door
        to: "on"
        for:
          minutes: 5
    action:
      - action: rest_command.matrix_clock_message
        data:
          message: "[warn] Front door open!"
          mode: 3          # Blink+Scroll
          brightness: 15
```

### 6.3 Morning briefing with weather and time

```yaml
automation:
  - alias: "Morning Briefing — Matrix Clock"
    trigger:
      - platform: time
        at: "07:00:00"
    condition:
      - condition: state
        entity_id: binary_sensor.matrix_clock_online
        state: "on"
    action:
      - action: rest_command.matrix_clock_message
        data:
          message: >-
            Good morning!
            {{ states('sensor.outdoor_temperature') }}°C outside.
          mode: 0
          scroll_speed_ms: 40
```

### 6.4 Reduce brightness at night automatically

```yaml
automation:
  - alias: "Matrix Clock — Night Brightness"
    trigger:
      - platform: time
        at: "22:00:00"
    action:
      - action: rest_command.matrix_clock_set_brightness
        data:
          brightness: 1

  - alias: "Matrix Clock — Day Brightness"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - action: rest_command.matrix_clock_set_brightness
        data:
          brightness: 8
```

> **Note:** the firmware also has a built-in auto-brightness-by-time-of-day feature (`night_brightness_start`, `night_brightness_end` in `POST /api/config`). Using the firmware-side feature is simpler; use HA automations only if you need finer control or want to tie brightness to other sensors (e.g. a lux sensor).

### 6.5 Notify when a person arrives home

```yaml
automation:
  - alias: "Person Arrived → Matrix Clock"
    trigger:
      - platform: state
        entity_id: person.jan
        to: "home"
    action:
      - action: rest_command.matrix_clock_message
        data:
          message: "[heart] Welcome home Jan!"
          mode: 1        # Blink
          duration_ms: 6000
          brightness: 12
```

### 6.6 Alert for high CO₂ or temperature sensor

```yaml
automation:
  - alias: "High CO2 → Matrix Clock"
    trigger:
      - platform: numeric_state
        entity_id: sensor.living_room_co2
        above: 1000
    action:
      - action: rest_command.matrix_clock_message
        data:
          message: "[warn] CO2 {{ states('sensor.living_room_co2') }} ppm!"
          mode: 3
          duration_ms: 30000
          brightness: 15
```

### 6.7 Timer countdown display

```yaml
automation:
  - alias: "Countdown Timer → Matrix Clock"
    trigger:
      - platform: state
        entity_id: timer.pizza
        to: "active"
    action:
      - action: rest_command.matrix_clock_message
        data:
          message: "Pizza timer started!"
          mode: 2
          duration_ms: 5000

  - alias: "Timer Finished → Matrix Clock"
    trigger:
      - platform: event
        event_type: timer.finished
        event_data:
          entity_id: timer.pizza
    action:
      - action: rest_command.matrix_clock_message_blink
        data:
          message: "[bell] PIZZA READY!"
          duration_ms: 15000
```

### 6.8 Disable display slots during working hours

```yaml
automation:
  - alias: "Work Hours — Quotes Only"
    trigger:
      - platform: time
        at: "08:00:00"
    condition:
      - condition: time
        weekday: [mon, tue, wed, thu, fri]
    action:
      - action: rest_command.matrix_clock_set_brightness
        data:
          brightness: 6
      # Optionally trigger a data refresh
      - delay: "00:00:02"
      - action: rest_command.matrix_clock_fetch
        data:
          slot: 3   # quotes
```

Add the `matrix_clock_fetch` REST command to `configuration.yaml`:

```yaml
rest_command:
  matrix_clock_fetch:
    url: "http://192.168.1.42/api/config"
    method: POST
    headers:
      Content-Type: application/json
    payload: '{"slot": {{ slot }} }'
    content_type: "application/json"
```

> For forcing a data refresh, use `POST /api/fetch` with `{"slot": 2}` (weather) or `{"slot": 3}` (quotes).

---

## 7. Dashboard card examples

### 7.1 Entities card — clock status

```yaml
type: entities
title: Matrix Clock
entities:
  - entity: sensor.matrix_clock_status
    name: Current Time
  - entity: binary_sensor.matrix_clock_online
    name: Online
  - entity: input_number.matrix_clock_brightness
    name: Brightness
```

### 7.2 Send a message from the dashboard — Button card

```yaml
type: button
name: "Ring Doorbell Alert"
icon: mdi:bell
tap_action:
  action: call-service
  service: rest_command.matrix_clock_message_blink
  service_data:
    message: "[bell] DOORBELL [bell]"
    duration_ms: 8000
```

### 7.3 Input text + button to send any message

1. Create an **Input Text** helper (Settings → Helpers → Text):
   - Name: `Matrix Clock Custom Message`
   - Entity ID: `input_text.matrix_clock_message`

2. Dashboard YAML card:

```yaml
type: vertical-stack
cards:
  - type: entities
    entities:
      - entity: input_text.matrix_clock_message
        name: Message
  - type: button
    name: Send to Display
    icon: mdi:led-strip
    tap_action:
      action: call-service
      service: rest_command.matrix_clock_message
      service_data:
        message: "{{ states('input_text.matrix_clock_message') }}"
        mode: 0
```

### 7.4 Brightness slider

```yaml
type: entities
title: Display Brightness
entities:
  - entity: input_number.matrix_clock_brightness
    name: Brightness
    icon: mdi:brightness-5
```

---

## 8. Troubleshooting

### `rest_command` returns `error` or no response

- Confirm the device IP is correct: `ping 192.168.1.42`
- Check Home Assistant logs (`Settings → System → Logs`) for connection-refused or timeout messages
- Verify the clock's web panel opens at `http://192.168.1.42` in a browser
- Make sure HA and the clock are on the **same network** (some VLANs or guest networks block inter-device communication)

### Sensor shows `unavailable`

- The `scan_interval` default is 30 s; wait one cycle after first deployment
- Increase the polling interval if the ESP32 is overloaded: `scan_interval: 60`
- Power-cycle the clock if it stopped responding (ESP32 can lock up after days of uptime without a watchdog reset)

### Message not appearing on the display

- The message slot has **top priority** but is **one-shot** — it shows once and is discarded. If you send another message before the first one finishes scrolling, only the latest is shown
- Make sure `mode` and `duration_ms` are correct for your use case; Scroll mode ignores `duration_ms`
- Check that `message` is not empty and is within the 127-character limit

### Icon tags not rendering

- Tags are case-sensitive: `[warn]` works, `[Warn]` does not
- Unknown tags are displayed as literal text (`[myicon]` → `[myicon]`); double-check the tag name against the reference table in [section 3.3](#33-available-icon-tags)

### Brightness not changing

- Range is `0–15` (integers only). Floats are accepted but cast internally; sending `7.5` becomes `7`
- The built-in auto-brightness schedule may override your setting if `night_brightness_start != night_brightness_end` is configured in the device

---

## Quick-reference: full `configuration.yaml` block

Copy this block as a starting point and replace `192.168.1.42` with your device's IP:

```yaml
# ─── Smart Matrix Clock ────────────────────────────────────────────────────────

rest_command:
  matrix_clock_message:
    url: "http://192.168.1.42/api/message"
    method: POST
    content_type: "application/json"
    payload: >-
      {
        "message":         "{{ message }}",
        "mode":            {{ mode            | default(0)  }},
        "brightness":      {{ brightness      | default(8)  }},
        "scroll_speed_ms": {{ scroll_speed_ms | default(50) }}
      }

  matrix_clock_message_blink:
    url: "http://192.168.1.42/api/message"
    method: POST
    content_type: "application/json"
    payload: >-
      {
        "message":     "{{ message }}",
        "mode":        1,
        "duration_ms": {{ duration_ms | default(5000) }},
        "brightness":  15
      }

  matrix_clock_set_brightness:
    url: "http://192.168.1.42/api/config"
    method: POST
    content_type: "application/json"
    payload: '{"brightness": {{ brightness }} }'

  matrix_clock_set_config:
    url: "http://192.168.1.42/api/config"
    method: POST
    content_type: "application/json"
    payload: "{{ payload }}"

  matrix_clock_fetch_weather:
    url: "http://192.168.1.42/api/fetch"
    method: POST
    content_type: "application/json"
    payload: '{"slot": 2}'

  matrix_clock_fetch_quotes:
    url: "http://192.168.1.42/api/fetch"
    method: POST
    content_type: "application/json"
    payload: '{"slot": 3}'

sensor:
  - platform: rest
    name: "Matrix Clock Status"
    unique_id: matrix_clock_status
    resource: "http://192.168.1.42/api/status"
    method: GET
    scan_interval: 30
    value_template: "{{ value_json.time_str }}"
    json_attributes:
      - ntp_synced
      - active_slot
      - ssid
      - ip
      - time_str

binary_sensor:
  - platform: template
    sensors:
      matrix_clock_online:
        friendly_name: "Matrix Clock Online"
        device_class: connectivity
        value_template: >-
          {{ states('sensor.matrix_clock_status') not in ['unavailable', 'unknown'] }}

input_number:
  matrix_clock_brightness:
    name: "Matrix Clock Brightness"
    min: 0
    max: 15
    step: 1
    mode: slider
    icon: mdi:brightness-5

automation:
  - alias: "Matrix Clock — Sync Brightness Helper"
    trigger:
      - platform: state
        entity_id: input_number.matrix_clock_brightness
    action:
      - action: rest_command.matrix_clock_set_brightness
        data:
          brightness: "{{ states('input_number.matrix_clock_brightness') | int }}"
```

---

Full REST API reference: [`docs/api-rest.md`](api-rest.md)
