#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

// Called every loop() iteration.
// Checks the weather and quotes fetch timers; performs the HTTP request when
// the interval fires.  Never blocks longer than the configured HTTP timeout.
void fetcherTick();

// Trigger an immediate weather fetch on the next fetcherTick() call.
// Useful after saving new coordinates or enabling the slot.
void fetcherReset();

// Trigger an immediate quotes fetch on the next fetcherTick() call.
// Useful after saving new tickers or enabling the slot.
void quotesFetcherReset();
