/**
 * haptic_feedback.c - Pebble-side haptic wrappers + heartbeat scheduler.
 *
 * Pattern selection lives in the portable core (haptic_patterns + feedback);
 * this file only translates the public API into semantic events and owns the
 * one genuinely platform-specific bit: the app_timer heartbeat loop.
 */

#include "haptic_feedback.h"

#include "feedback.h"
#include "haptic_patterns.h"
#include "pebble_watch.h"

#include <pebble.h> // AppTimer / app_timer_* for the heartbeat loop

static AppTimer *s_heartbeat_timer = NULL;
static bool s_is_rapid = false;

void haptic_init(void) {
  pebble_watch_install();
  s_heartbeat_timer = NULL;
  s_is_rapid = false;
}

void haptic_deinit(void) { haptic_stop_heartbeat(); }

void haptic_pulse_soft(void) { feedback_fire(HAPTIC_HEARTBEAT_SOFT); }

void haptic_pulse_rapid(void) { feedback_fire(HAPTIC_HEARTBEAT_RAPID); }

void haptic_pulse_danger(void) { feedback_fire(HAPTIC_DANGER); }

void haptic_jump_scare(void) { feedback_fire(HAPTIC_JUMP_SCARE); }

void haptic_pace_too_slow(void) { feedback_fire(HAPTIC_PACE_TOO_SLOW); }

void haptic_pace_too_fast(void) { feedback_fire(HAPTIC_PACE_TOO_FAST); }

void haptic_hr_too_high(void) { feedback_fire(HAPTIC_HR_TOO_HIGH); }

void haptic_hr_too_low(void) { feedback_fire(HAPTIC_HR_TOO_LOW); }

void haptic_segment_change(void) { feedback_fire(HAPTIC_SEGMENT_CHANGE); }

void haptic_fire_alert(AlertType alert) {
  feedback_fire(haptic_event_for_alert(alert));
}

static void heartbeat_timer_callback(void *data) {
  (void)data;
  feedback_fire(s_is_rapid ? HAPTIC_HEARTBEAT_RAPID : HAPTIC_HEARTBEAT_SOFT);
  uint32_t interval =
      s_is_rapid ? HAPTIC_HEARTBEAT_RAPID_MS : HAPTIC_HEARTBEAT_SOFT_MS;
  s_heartbeat_timer =
      app_timer_register(interval, heartbeat_timer_callback, NULL);
}

void haptic_start_heartbeat(bool behind) {
  haptic_stop_heartbeat();
  s_is_rapid = behind;

  feedback_fire(behind ? HAPTIC_HEARTBEAT_RAPID : HAPTIC_HEARTBEAT_SOFT);

  uint32_t interval =
      behind ? HAPTIC_HEARTBEAT_RAPID_MS : HAPTIC_HEARTBEAT_SOFT_MS;
  s_heartbeat_timer =
      app_timer_register(interval, heartbeat_timer_callback, NULL);
}

void haptic_stop_heartbeat(void) {
  if (s_heartbeat_timer != NULL) {
    app_timer_cancel(s_heartbeat_timer);
    s_heartbeat_timer = NULL;
  }
  feedback_cancel();
}
