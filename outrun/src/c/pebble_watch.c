/**
 * pebble_watch.c - Pebble SDK implementation of WatchInterface.
 *
 * One of the few files that touches the vibration / clock SDK. Heart-rate
 * reads delegate to hr_monitor (the Pebble Health adapter). Everything
 * portable reaches these through watch().
 */

#include "pebble_watch.h"

#include "hr_monitor.h"
#include "watch_interface.h"

#include <pebble.h>

static void pebble_play(const HapticPattern *pattern) {
  if (!pattern || !pattern->durations || pattern->count == 0) {
    return;
  }
  VibePattern vibe = {
      .durations = pattern->durations,
      .num_segments = pattern->count,
  };
  vibes_enqueue_custom_pattern(vibe);
}

static void pebble_cancel(void) { vibes_cancel(); }

static uint32_t pebble_now_seconds(void) { return (uint32_t)time(NULL); }

static uint8_t pebble_heart_rate(void) { return hr_monitor_get_bpm(); }

static bool pebble_heart_rate_available(void) {
  return hr_monitor_is_available();
}

static void pebble_heart_rate_start(void) { hr_monitor_start(); }

static void pebble_heart_rate_stop(void) { hr_monitor_stop(); }

static const WatchInterface PEBBLE_WATCH = {
    .play = pebble_play,
    .cancel = pebble_cancel,
    .now_seconds = pebble_now_seconds,
    .heart_rate = pebble_heart_rate,
    .heart_rate_available = pebble_heart_rate_available,
    .heart_rate_start = pebble_heart_rate_start,
    .heart_rate_stop = pebble_heart_rate_stop,
};

void pebble_watch_install(void) { watch_set(&PEBBLE_WATCH); }
