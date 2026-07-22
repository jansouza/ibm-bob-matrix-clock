#pragma once

#include <ESPAsyncWebServer.h>

// Register all HTTP routes on the given server instance.
// Must be called once in setup() after wifiBegin().
void webRoutesBegin(AsyncWebServer& server);
