/**
 * mock_watch.h - Host-test implementation of WatchInterface.
 *
 * Records vibration calls and exposes a controllable clock / heart rate so
 * portable logic can be exercised with no SDK.
 */

#ifndef MOCK_WATCH_H
#define MOCK_WATCH_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_interface.h"

/** Install this mock as the active watch. */
void mock_watch_install(void);

/** Clear all recorded state and reset the clock to 0. */
void mock_watch_reset(void);

/** Advance the mock clock by N seconds. */
void mock_watch_advance(uint32_t seconds);

/** Set the heart rate the mock reports. */
void mock_watch_set_hr(uint8_t bpm, bool available);

/** Inspection helpers. */
uint32_t mock_watch_play_count(void);
uint32_t mock_watch_cancel_count(void);
const HapticPattern *mock_watch_last_pattern(void);

#endif // MOCK_WATCH_H
