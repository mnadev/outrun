/**
 * haptic_feedback.c - Horror-themed vibration patterns
 */

#include "haptic_feedback.h"

// Timer for continuous heartbeat
static AppTimer *s_heartbeat_timer = NULL;
static bool s_is_rapid = false;

// Custom vibe patterns
// Soft pulse: short-pause-short (lub-dub)
static const uint32_t SOFT_PULSE_SEGMENTS[] = {50, 100, 50};
static const VibePattern SOFT_PULSE_PATTERN = {
    .durations = SOFT_PULSE_SEGMENTS,
    .num_segments = ARRAY_LENGTH(SOFT_PULSE_SEGMENTS)};

// Rapid heartbeat: quick double-pulses
static const uint32_t RAPID_PULSE_SEGMENTS[] = {30, 50, 30, 50, 30};
static const VibePattern RAPID_PULSE_PATTERN = {
    .durations = RAPID_PULSE_SEGMENTS,
    .num_segments = ARRAY_LENGTH(RAPID_PULSE_SEGMENTS)};

// Danger: intense rapid pattern
static const uint32_t DANGER_PULSE_SEGMENTS[] = {50, 30, 50, 30, 50, 30, 50};
static const VibePattern DANGER_PULSE_PATTERN = {
    .durations = DANGER_PULSE_SEGMENTS,
    .num_segments = ARRAY_LENGTH(DANGER_PULSE_SEGMENTS)};

// Jump scare: long aggressive burst
static const uint32_t JUMP_SCARE_SEGMENTS[] = {200, 100, 300, 100, 200};
static const VibePattern JUMP_SCARE_PATTERN = {
    .durations = JUMP_SCARE_SEGMENTS,
    .num_segments = ARRAY_LENGTH(JUMP_SCARE_SEGMENTS)};

// Forward declaration
static void heartbeat_timer_callback(void *data);

void haptic_init(void) {
  s_heartbeat_timer = NULL;
  s_is_rapid = false;
}

void haptic_deinit(void) { haptic_stop_heartbeat(); }

void haptic_pulse_soft(void) {
  vibes_enqueue_custom_pattern(SOFT_PULSE_PATTERN);
}

void haptic_pulse_rapid(void) {
  vibes_enqueue_custom_pattern(RAPID_PULSE_PATTERN);
}

void haptic_pulse_danger(void) {
  vibes_enqueue_custom_pattern(DANGER_PULSE_PATTERN);
}

void haptic_jump_scare(void) {
  vibes_enqueue_custom_pattern(JUMP_SCARE_PATTERN);
}

static void heartbeat_timer_callback(void *data) {
  if (s_is_rapid) {
    haptic_pulse_rapid();
    // Rapid: pulse every 500ms (120 BPM)
    s_heartbeat_timer = app_timer_register(500, heartbeat_timer_callback, NULL);
  } else {
    haptic_pulse_soft();
    // Soft: pulse every 5000ms (calm reminder)
    s_heartbeat_timer =
        app_timer_register(5000, heartbeat_timer_callback, NULL);
  }
}

void haptic_start_heartbeat(bool behind) {
  haptic_stop_heartbeat();
  s_is_rapid = behind;

  // Initial pulse
  if (behind) {
    haptic_pulse_rapid();
  } else {
    haptic_pulse_soft();
  }

  // Schedule next
  int interval = behind ? 500 : 5000;
  s_heartbeat_timer =
      app_timer_register(interval, heartbeat_timer_callback, NULL);
}

void haptic_stop_heartbeat(void) {
  if (s_heartbeat_timer != NULL) {
    app_timer_cancel(s_heartbeat_timer);
    s_heartbeat_timer = NULL;
  }
  vibes_cancel();
}
