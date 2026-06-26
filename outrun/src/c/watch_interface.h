/**
 * watch_interface.h - The single platform port (output + input).
 *
 * This vtable is the ONLY seam between portable logic and a specific
 * watch OS. Fill it in per platform (Pebble today; Garmin Connect IQ or
 * Wear OS later) and inject it once at startup with watch_set(). Portable
 * code never calls an SDK directly -- it goes through watch() or the
 * convenience accessors below.
 *
 * Keep this file free of <pebble.h>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "haptic_patterns.h"

typedef struct {
  // --- Output ---
  // Play one vibration pattern (fire-and-forget).
  void (*play)(const HapticPattern *pattern);
  // Cancel/stop any in-progress or queued vibration.
  void (*cancel)(void);

  // --- Input ---
  // Monotonic-ish wall clock in seconds, used for run timing.
  uint32_t (*now_seconds)(void);
  // Current heart rate in BPM (0 if unknown).
  uint8_t (*heart_rate)(void);
  // Whether a heart-rate source is currently usable.
  bool (*heart_rate_available)(void);
} WatchInterface;

/** Inject the active platform implementation (call once at startup). */
void watch_set(const WatchInterface *impl);

/** Get the active implementation (NULL until watch_set() is called). */
const WatchInterface *watch(void);

// Null-safe convenience accessors over the active interface.
uint32_t watch_now_seconds(void);
uint8_t watch_heart_rate(void);
bool watch_heart_rate_available(void);
