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
#include "stalker_themes.h"

#include <pebble.h> // AppTimer / app_timer_* for the heartbeat loop

static AppTimer *s_heartbeat_timer = NULL;
static bool s_is_rapid = false;

void haptic_init(void) {
  pebble_watch_install();
  s_heartbeat_timer = NULL;
  s_is_rapid = false;
}

void haptic_deinit(void) { haptic_stop_heartbeat(); }

void haptic_jump_scare(void) { themes_haptic_scare(); }

void haptic_segment_change(void) { feedback_fire(HAPTIC_SEGMENT_CHANGE); }

void haptic_fire_alert(AlertType alert) {
  feedback_fire(haptic_event_for_alert(alert));
}

// The heartbeat loop beat: short and guaranteed to finish well before the
// next enqueue, so vibes never queues patterns back-to-back into continuous
// vibration. The theme "danger" pattern (up to ~1.1s) is too long to repeat
// at the rapid cadence, so it fires once on entry (haptic_start_heartbeat)
// and the loop uses the portable rapid thump instead.
static void play_heartbeat_pulse(void) {
  if (s_is_rapid) {
    feedback_fire(HAPTIC_HEARTBEAT_RAPID);
  } else {
    themes_haptic_pulse();
  }
}

static void heartbeat_timer_callback(void *data) {
  (void)data;
  play_heartbeat_pulse();
  uint32_t interval =
      s_is_rapid ? HAPTIC_HEARTBEAT_RAPID_MS : HAPTIC_HEARTBEAT_SOFT_MS;
  s_heartbeat_timer =
      app_timer_register(interval, heartbeat_timer_callback, NULL);
}

void haptic_start_heartbeat(bool behind) {
  haptic_stop_heartbeat();
  s_is_rapid = behind;

  // One theme-voiced cue when the heartbeat level changes: the stalker's
  // danger pattern as you enter the danger zone, its pulse when you're merely
  // behind. The loop then takes over with short, non-overlapping beats.
  if (behind) {
    themes_haptic_danger();
  } else {
    themes_haptic_pulse();
  }

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
