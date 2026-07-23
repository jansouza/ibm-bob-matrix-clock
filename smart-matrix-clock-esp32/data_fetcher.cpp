/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "data_fetcher.h"
#include "config.h"
#include "globals.h"
#include "locale_data.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>

// ─── Internal state ───────────────────────────────────────────────────────────

// Initialize to a value that forces a fetch on the first fetcherTick() call
// after WiFi connects (millis() is typically <1 s at boot, so subtracting
// WEATHER_UPDATE_DEFAULT_MS underflows but unsigned arithmetic makes it work:
// now(small) - (0 - WEATHER_UPDATE_DEFAULT_MS) = now + WEATHER_UPDATE_DEFAULT_MS
// which is >= cfgWeatherUpdateMs immediately).
// Simplest approach: start at 0 and treat 0 as "never fetched" to force first fetch.
static uint32_t _lastWeatherFetch = 0;   // millis() of last fetch attempt; 0 = never
static bool     _forceFetch       = true; // force fetch on first enabled tick

// ─── fetchWeather ─────────────────────────────────────────────────────────────

static void _fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Determine unit string for Open-Meteo
    // Celsius is the default; Fahrenheit requires &temperature_unit=fahrenheit
    bool isFahrenheit = (cfgTempUnit[0] == 'F' || cfgTempUnit[0] == 'f');

    // Build URL
    // Fields: temperature_2m (current), weathercode, temperature_2m_max,
    //         temperature_2m_min (daily).  Both current and daily are requested
    //         in one call so we get everything in a single HTTP round-trip.
    char url[256];
    if (isFahrenheit) {
        snprintf(url, sizeof(url),
            "http://api.open-meteo.com/v1/forecast"
            "?latitude=%.4f&longitude=%.4f"
            "&current=temperature_2m,weathercode"
            "&daily=temperature_2m_max,temperature_2m_min"
            "&temperature_unit=fahrenheit"
            "&forecast_days=1&timezone=auto",
            cfgWeatherLat, cfgWeatherLon);
    } else {
        snprintf(url, sizeof(url),
            "http://api.open-meteo.com/v1/forecast"
            "?latitude=%.4f&longitude=%.4f"
            "&current=temperature_2m,weathercode"
            "&daily=temperature_2m_max,temperature_2m_min"
            "&forecast_days=1&timezone=auto",
            cfgWeatherLat, cfgWeatherLon);
    }

    HTTPClient http;
    http.setTimeout(5000);   // 5 s timeout — hard rule from AGENTS.md
    http.begin(url);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[Weather] HTTP error %d\n", code);
        http.end();
        if (weatherCache.valid) weatherCache.stale = true;
        return;
    }

    // Read the full body into a String before parsing.
    // getStream() is unreliable with chunked/compressed responses on ESP32 —
    // the WiFiClient may not have all bytes ready when deserializeJson starts
    // reading, producing an InvalidInput error on a valid payload.
    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);

    if (err) {
        Serial.printf("[Weather] JSON parse error: %s  body[0..80]: %.80s\n",
                      err.c_str(), body.c_str());
        if (weatherCache.valid) weatherCache.stale = true;
        return;
    }

    // Extract values.
    // Open-Meteo API uses "weather_code" (underscore) in current API versions.
    float temp    = doc["current"]["temperature_2m"]  | 0.0f;
    int   wmoCode = doc["current"]["weather_code"]    | 0;
    float tmax    = doc["daily"]["temperature_2m_max"][0] | 0.0f;
    float tmin    = doc["daily"]["temperature_2m_min"][0] | 0.0f;

    // Determine language for the condition string
    uint8_t lang = (cfgLanguage[0] == 'p' || cfgLanguage[0] == 'P') ? LANG_PT : LANG_EN;
    const char* cond = weatherConditionName(lang, (uint8_t)wmoCode);

    // Update cache
    weatherCache.temp      = temp;
    weatherCache.maxTemp   = tmax;
    weatherCache.minTemp   = tmin;
    strncpy(weatherCache.condition, cond, WEATHER_CONDITION_MAX - 1);
    weatherCache.condition[WEATHER_CONDITION_MAX - 1] = '\0';
    weatherCache.valid     = true;
    weatherCache.stale     = false;
    weatherCache.fetchedAt = millis();

    Serial.printf("[Weather] %.1f%c %s  Min%.1f Max%.1f\n",
                  temp, isFahrenheit ? 'F' : 'C',
                  cond, tmin, tmax);
}

// ─── Public API ───────────────────────────────────────────────────────────────

void fetcherReset() {
    _forceFetch = true;
}

void fetcherTick() {
    // Only fetch if the weather slot is enabled
    if (!slotEnabled[2]) return;

    uint32_t now = millis();
    bool timerFired = (now - _lastWeatherFetch >= cfgWeatherUpdateMs);

    if (_forceFetch || timerFired) {
        _lastWeatherFetch = now;
        _forceFetch       = false;
        _fetchWeather();
    }
}
