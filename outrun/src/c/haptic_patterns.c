/**
 * haptic_patterns.c - Pattern data + lookups (pure, no SDK).
 */

#include "haptic_patterns.h"

#include <stddef.h>

#define PATTERN_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Pace band alerts.
static const uint32_t PACE_SLOW[] = {80, 60, 80, 60, 80};
static const uint32_t PACE_FAST[] = {250};

// Heart-rate band alerts.
static const uint32_t HR_HIGH[] = {200, 80, 200};
static const uint32_t HR_LOW[] = {350, 80, 100};

// Plan segment transition.
static const uint32_t SEGMENT_CHANGE[] = {80, 60, 200, 60, 80};

// Attention grabbers / horror theme.
static const uint32_t JUMP_SCARE[] = {200, 100, 300, 100, 200};
static const uint32_t HEARTBEAT_SOFT[] = {50, 100, 50};
static const uint32_t HEARTBEAT_RAPID[] = {30, 50, 30, 50, 30};
static const uint32_t DANGER[] = {50, 30, 50, 30, 50, 30, 50};

#define PAT(arr)                                                               \
  { .durations = (arr), .count = (uint32_t)PATTERN_COUNT(arr) }

static const HapticPattern PATTERNS[HAPTIC_EVENT_COUNT] = {
    [HAPTIC_NONE] = {NULL, 0},
    [HAPTIC_PACE_TOO_SLOW] = PAT(PACE_SLOW),
    [HAPTIC_PACE_TOO_FAST] = PAT(PACE_FAST),
    [HAPTIC_HR_TOO_HIGH] = PAT(HR_HIGH),
    [HAPTIC_HR_TOO_LOW] = PAT(HR_LOW),
    [HAPTIC_SEGMENT_CHANGE] = PAT(SEGMENT_CHANGE),
    [HAPTIC_JUMP_SCARE] = PAT(JUMP_SCARE),
    [HAPTIC_HEARTBEAT_SOFT] = PAT(HEARTBEAT_SOFT),
    [HAPTIC_HEARTBEAT_RAPID] = PAT(HEARTBEAT_RAPID),
    [HAPTIC_DANGER] = PAT(DANGER),
};

const HapticPattern *haptic_pattern_for(HapticEvent event) {
  if (event <= HAPTIC_NONE || event >= HAPTIC_EVENT_COUNT) {
    return NULL;
  }
  const HapticPattern *pattern = &PATTERNS[event];
  return pattern->durations ? pattern : NULL;
}

HapticEvent haptic_event_for_alert(AlertType alert) {
  switch (alert) {
  case ALERT_PACE_TOO_SLOW:
    return HAPTIC_PACE_TOO_SLOW;
  case ALERT_PACE_TOO_FAST:
    return HAPTIC_PACE_TOO_FAST;
  case ALERT_HR_TOO_HIGH:
    return HAPTIC_HR_TOO_HIGH;
  case ALERT_HR_TOO_LOW:
    return HAPTIC_HR_TOO_LOW;
  case ALERT_NONE:
  default:
    return HAPTIC_NONE;
  }
}
