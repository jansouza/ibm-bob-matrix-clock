#include "config.h"
#include "globals.h"
#include "display.h"
#include "wifi_manager.h"
#include "ntp.h"
#include "persistence.h"
#include "web_routes.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// ─── Web server instance ──────────────────────────────────────────────────────
static AsyncWebServer _server(80);

// ─────────────────────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // 1. Check BOOT button (GPIO 0 active-LOW) for factory reset
    pinMode(PIN_BOOT, INPUT_PULLUP);
    if (digitalRead(PIN_BOOT) == LOW) {
        Serial.println("[Boot] BOOT button held — factory reset");
        factoryReset();
        // Continue to AP mode with cleared config
    }

    // 2. Load persisted settings from NVS (fills cfg* globals, including brightness)
    loadConfig();

    // 3. Initialise display — uses currentBrightness already loaded from NVS
    displayBegin();

    // 4. Connect to saved WiFi; falls back to setup AP if no credentials / timeout
    wifiBegin();

    // 5. Start NTP only if we are in station mode (skip in AP-only mode)
    if (WiFi.status() == WL_CONNECTED) {
        ntpBegin(cfgNtpServer, nullptr);
    }

    // 6. Apply saved timezone AFTER ntpBegin() — configTime() resets TZ to UTC
    //    internally, so applyTimezone() must run last to override it correctly.
    applyTimezone();

    // 7. Register HTTP routes and start the web server
    webRoutesBegin(_server);
    _server.begin();
    Serial.println("[Web] HTTP server started on port 80");

    // 8. Show IP on the display so the user knows where to connect.
    //    In AP mode the message loops until the device leaves AP mode.
    {
        char ipMsg[48];
        if (WiFi.status() == WL_CONNECTED) {
            snprintf(ipMsg, sizeof(ipMsg), "IP %s", WiFi.localIP().toString().c_str());
            Serial.printf("[Boot] Display IP: %s\n", ipMsg);
            displayScrollText(ipMsg);       // one-shot scroll, then clock resumes
        } else {
            snprintf(ipMsg, sizeof(ipMsg), "Config %s", WiFi.softAPIP().toString().c_str());
            Serial.printf("[Boot] Display AP: %s\n", ipMsg);
            displaySetApMessage(ipMsg);     // loops until station connects
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// loop()
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    wifiTick();     // handle reconnect and deferred restart
    ntpTick();      // check sync status, trigger periodic re-sync
    displayTick();  // update colon blink, date scroll, alert queue
}
