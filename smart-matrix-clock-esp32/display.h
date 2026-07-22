#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */
#include <stdint.h>

// Initialise the MAX72XX driver and Parola controller.
// Must be called once in setup().
void displayBegin();

// Called every loop() iteration.
// Manages colon blink, date scroll timer, alert queue, and non-blocking scroll.
void displayTick();

// Change display brightness at runtime (0–15).
void setBrightness(uint8_t level);

// Change scroll frame interval at runtime (ms).
void setScrollSpeed(uint16_t ms);

// Immediately start a non-blocking left-scroll of the given Latin-1 string.
// Any current time display is interrupted; clock resumes after scroll ends.
void displayScrollText(const char* latin1Text);

// Set the AP config message (Latin-1) and start scrolling it immediately.
// While isApMode() returns true the message will repeat indefinitely.
void displaySetApMessage(const char* latin1Text);
