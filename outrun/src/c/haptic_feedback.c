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

static const uint32_t PACE_SLOW_SEGMENTS[] = {80, 60, 80, 60, 80};
static const VibePattern PACE_SLOW_PATTERN = {
    .durations = PACE_SLOW_SEGMENTS,
    .num_segments = ARRAY_LENGTH(PACE_SLOW_SEGMENTS)};

static const uint32_t PACE_FAST_SEGMENTS[] = {250};
static const VibePattern PACE_FAST_PATTERN = {
    .durations = PACE_FAST_SEGMENTS,
    .num_segments = ARRAY_LENGTH(PACE_FAST_SEGMENTS)};

static const uint32_t HR_HIGH_SEGMENTS[] = {200, 80, 200};
static const VibePattern HR_HIGH_PATTERN = {
    .durations = HR_HIGH_SEGMENTS,
    .num_segments = ARRAY_LENGTH(HR_HIGH_SEGMENTS)};

static const uint32_t HR_LOW_SEGMENTS[] = {350, 80, 100};
static const VibePattern HR_LOW_PATTERN = {
    .durations = HR_LOW_SEGMENTS,
    .num_segments = ARRAY_LENGTH(HR_LOW_SEGMENTS)};

static const uint32_t SEGMENT_CHANGE_SEGMENTS[] = {80, 60, 200, 60, 80};
static const VibePattern SEGMENT_CHANGE_PATTERN = {
    .durations = SEGMENT_CHANGE_SEGMENTS,
    .num_segments = ARRAY_LENGTH(SEGMENT_CHANGE_SEGMENTS)};

void haptic_pace_too_slow(void) {
  vibes_enqueue_custom_pattern(PACE_SLOW_PATTERN);
}

void haptic_pace_too_fast(void) {
  vibes_enqueue_custom_pattern(PACE_FAST_PATTERN);
}

void haptic_hr_too_high(void) {
  vibes_enqueue_custom_pattern(HR_HIGH_PATTERN);
}

void haptic_hr_too_low(void) {
  vibes_enqueue_custom_pattern(HR_LOW_PATTERN);
}

void haptic_segment_change(void) {
  vibes_enqueue_custom_pattern(SEGMENT_CHANGE_PATTERN);
}

void haptic_fire_alert(AlertType alert) {
  switch (alert) {
  case ALERT_PACE_TOO_SLOW:
    haptic_pace_too_slow();
    break;
  case ALERT_PACE_TOO_FAST:
    haptic_pace_too_fast();
    break;
  case ALERT_HR_TOO_HIGH:
    haptic_hr_too_high();
    break;
  case ALERT_HR_TOO_LOW:
    haptic_hr_too_low();
    break;
  default:
    break;
  }
}
