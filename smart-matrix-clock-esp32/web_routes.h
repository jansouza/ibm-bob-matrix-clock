#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include <ESPAsyncWebServer.h>

// Register all HTTP routes on the given server instance.
// Must be called once in setup() after wifiBegin().
void webRoutesBegin(AsyncWebServer& server);
