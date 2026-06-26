/**
 * haptic_patterns.h - Portable haptic pattern definitions.
 *
 * Maps semantic feedback events to vibration patterns (millisecond
 * on/off segments, starting with vibe-on). This is pure data plus a
 * lookup, with NO SDK dependency, so every platform adapter plays
 * byte-for-byte identical patterns. Keep this file free of <pebble.h>.
 */

#pragma once

#include <stdint.h>

#include "alert_engine.h" // for AlertType

// Cadence for the continuous heartbeat loop (milliseconds between pulses).
#define HAPTIC_HEARTBEAT_SOFT_MS 5000
#define HAPTIC_HEARTBEAT_RAPID_MS 500

// Semantic feedback events. The portable core speaks in these; adapters
// never need to know what any given event "feels" like.
typedef enum {
  HAPTIC_NONE = 0,
  HAPTIC_PACE_TOO_SLOW,
  HAPTIC_PACE_TOO_FAST,
  HAPTIC_HR_TOO_HIGH,
  HAPTIC_HR_TOO_LOW,
  HAPTIC_SEGMENT_CHANGE,
  HAPTIC_JUMP_SCARE,
  HAPTIC_HEARTBEAT_SOFT,
  HAPTIC_HEARTBEAT_RAPID,
  HAPTIC_DANGER,
  HAPTIC_EVENT_COUNT
} HapticEvent;

// A vibration pattern: alternating vibe-on / pause durations in ms,
// beginning with a vibe-on segment.
typedef struct {
  const uint32_t *durations;
  uint32_t count;
} HapticPattern;

/**
 * Look up the pattern for an event.
 * @return pointer to a static pattern, or NULL for HAPTIC_NONE / unknown.
 */
const HapticPattern *haptic_pattern_for(HapticEvent event);

/**
 * Translate a debounced pace/HR AlertType into its haptic event.
 * Returns HAPTIC_NONE for ALERT_NONE.
 */
HapticEvent haptic_event_for_alert(AlertType alert);
