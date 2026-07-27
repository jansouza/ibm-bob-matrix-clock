#pragma once
/*
 * Host-test stub layer — replaces Arduino / ESP32 headers so that the
 * firmware's pure-logic .cpp files compile cleanly on the development host.
 *
 * Only the symbols actually referenced by the modules under test are defined
 * here.  Do NOT add Arduino emulation beyond what is needed.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// ─── pgm_read_byte / PROGMEM ──────────────────────────────────────────────────
// On the host, PROGMEM data lives in normal RAM — just dereference the pointer.
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif

// ─── nullptr for C files compiled as C++ ──────────────────────────────────────
// (already available in C++11 but some included headers define their own guard)

// ─── Arduino String class — minimal stub ─────────────────────────────────────
// Only needed if locale_data.cpp or text_encoding.cpp pull it in transitively.
// They don't — this stub exists as a safeguard.

// ─── Serial stub (locale_data.cpp does not use Serial; data_fetcher does) ────
struct _SerialStub {
    template<typename... Args> void printf(Args...) {}
    template<typename T>       void println(T)      {}
    template<typename T>       void print(T)        {}
};
extern _SerialStub Serial;
