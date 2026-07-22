#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */
#include <stdint.h>
#include <stdbool.h>

// Try to connect to the saved WiFi credentials (cfgWifiSsid / cfgWifiPass).
// If credentials are empty or connection times out, opens the setup AP instead.
// Blocks in setup() — OK to use blocking delay() here.
void wifiBegin();

// Called every loop() iteration.
// If the station is connected, does nothing.
// If disconnected, attempts to reconnect every WIFI_RECONNECT_MS ms.
void wifiTick();

// Schedule a deferred ESP.restart() to fire after delayMs milliseconds.
// The actual restart happens inside wifiTick() once the delay has elapsed.
// This ensures the HTTP response has time to reach the client before restarting.
void scheduleRestart(uint32_t delayMs);

// Returns true if a restart is currently scheduled.
bool isRestartPending();

// Returns true if the device is currently in AP (setup) mode.
bool isApMode();
