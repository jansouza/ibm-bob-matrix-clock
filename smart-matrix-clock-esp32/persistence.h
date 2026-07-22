#pragma once
#include <stdint.h>
#include <stdbool.h>

// Load all persisted settings from NVS into global config vars.
// Missing keys fall back to compile-time defaults from config.h.
// Must be called before wifiBegin() and ntpBegin().
void loadConfig();

// Persist all current global config vars back to NVS.
void saveConfig();

// Translate the current cfgTimezone (IANA name) to a POSIX TZ string and
// apply it with setenv("TZ",...) + tzset().
// Safe to call repeatedly; harmless if the timezone has not changed.
void applyTimezone();

// Erase the entire NVS namespace ("clk") and reset RAM globals to defaults.
// Does NOT restart the device — caller is responsible for that.
void factoryReset();

// Generate a new random 32-hex-char API key and store it in cfgApiKey[].
// Does NOT save to NVS — call saveConfig() after if persistence is needed.
void generateApiKey();

// Returns true if `code` is one of the web UI languages the firmware knows
// about (see the table in persistence.cpp). Add new languages there.
bool isUiLanguageValid(const char* code);
