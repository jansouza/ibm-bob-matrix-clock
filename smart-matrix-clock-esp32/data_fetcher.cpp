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

static uint32_t _lastQuotesFetch  = 0;   // millis() of last quotes fetch attempt; 0 = never
static bool     _forceQuotesFetch = true; // force fetch on first enabled tick

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

// ─── fetchQuotes ──────────────────────────────────────────────────────────────

// Split cfgQuotesTickers ("PETR4.SA,AAPL") into up to QUOTES_MAX_TICKERS
// symbols. Returns the number of symbols found.
static uint8_t _splitTickers(char (*symbols)[QUOTES_SYMBOL_MAX]) {
    char buf[QUOTES_TICKERS_MAX];
    strncpy(buf, cfgQuotesTickers, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    uint8_t count = 0;
    char* saveptr = nullptr;
    char* tok = strtok_r(buf, ",", &saveptr);
    while (tok != nullptr && count < QUOTES_MAX_TICKERS) {
        // Trim leading/trailing whitespace
        while (*tok == ' ') tok++;
        char* end = tok + strlen(tok) - 1;
        while (end > tok && *end == ' ') { *end = '\0'; end--; }

        if (tok[0] != '\0') {
            strncpy(symbols[count], tok, QUOTES_SYMBOL_MAX - 1);
            symbols[count][QUOTES_SYMBOL_MAX - 1] = '\0';
            count++;
        }
        tok = strtok_r(nullptr, ",", &saveptr);
    }
    return count;
}

// Fetch a single symbol from the chart endpoint (no auth/crumb required,
// unlike v7/finance/quote which now 401s without a session cookie).
// Returns true and fills price/changePercent on success.
static bool _fetchOneQuote(const char* symbol, float* price, float* changePercent) {
    char url[192];
    snprintf(url, sizeof(url),
        "https://query1.finance.yahoo.com/v8/finance/chart/%s", symbol);

    HTTPClient http;
    http.setTimeout(5000);   // 5 s timeout — hard rule from AGENTS.md
    http.begin(url);
    // Yahoo rejects requests with no User-Agent (or the default ESP32 one).
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[Quotes] %s HTTP error %d\n", symbol, code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[Quotes] %s JSON parse error: %s\n", symbol, err.c_str());
        return false;
    }

    JsonObject meta = doc["chart"]["result"][0]["meta"];
    if (meta.isNull() || !meta["regularMarketPrice"].is<float>()) {
        Serial.printf("[Quotes] %s: no meta in response\n", symbol);
        return false;
    }

    float last = meta["regularMarketPrice"].as<float>();
    float prevClose = meta["previousClose"] | meta["chartPreviousClose"] | last;

    *price = last;
    *changePercent = (prevClose != 0.0f) ? ((last - prevClose) / prevClose * 100.0f) : 0.0f;
    return true;
}

static void _fetchQuotes() {
    if (WiFi.status() != WL_CONNECTED) return;

    char symbols[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];
    uint8_t count = _splitTickers(symbols);
    if (count == 0) return;

    uint8_t newCount = 0;
    bool anyFailed = false;

    for (uint8_t i = 0; i < count; i++) {
        float price, changePercent;
        if (_fetchOneQuote(symbols[i], &price, &changePercent)) {
            strncpy(quoteCache[newCount].symbol, symbols[i], QUOTES_SYMBOL_MAX - 1);
            quoteCache[newCount].symbol[QUOTES_SYMBOL_MAX - 1] = '\0';
            quoteCache[newCount].price         = price;
            quoteCache[newCount].changePercent = changePercent;
            quoteCache[newCount].valid         = true;
            newCount++;
        } else {
            anyFailed = true;
        }
    }

    if (newCount == 0) {
        if (quoteCacheCount > 0) quotesCacheStale = true;
        return;
    }

    quoteCacheCount  = newCount;
    // Partial failure (some symbols fetched, some didn't) still counts as
    // stale so the panel/display flag the data as incomplete.
    quotesCacheStale = anyFailed;

    Serial.printf("[Quotes] Updated %u/%u symbols\n", newCount, count);
}

// ─── Public API ───────────────────────────────────────────────────────────────

void fetcherReset() {
    _forceFetch = true;
}

void quotesFetcherReset() {
    _forceQuotesFetch = true;
}

void fetcherTick() {
    uint32_t now = millis();

    // Only fetch if the weather slot is enabled
    if (slotEnabled[2]) {
        bool timerFired = (now - _lastWeatherFetch >= cfgWeatherUpdateMs);
        if (_forceFetch || timerFired) {
            _lastWeatherFetch = now;
            _forceFetch       = false;
            _fetchWeather();
        }
    }

    // Only fetch if the quotes slot is enabled
    if (slotEnabled[3]) {
        bool timerFired = (now - _lastQuotesFetch >= cfgQuotesUpdateMs);
        if (_forceQuotesFetch || timerFired) {
            _lastQuotesFetch  = now;
            _forceQuotesFetch = false;
            _fetchQuotes();
        }
    }
}
