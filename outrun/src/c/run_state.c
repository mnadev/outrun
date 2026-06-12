/**
 * run_state.c - Run state machine implementation
 */

#include "run_state.h"
#include <pebble.h>

static RunState s_state = RUN_IDLE;
static RunStats s_stats;
static time_t s_start_time;
static uint32_t s_paused_elapsed;

#ifdef RUN_STATE_HOST_TEST
static time_t s_test_time_base = 1000000;
static time_t s_test_time_offset = 0;

static time_t run_state_now(void) {
  return s_test_time_base + s_test_time_offset;
}
#else
static time_t run_state_now(void) { return time(NULL); }
#endif

void run_state_init(void) {
  s_state = RUN_IDLE;
  s_stats.elapsed_seconds = 0;
  s_stats.distance_meters = 0;
  s_stats.avg_pace_sec_per_km = 0;
  s_stats.avg_hr_bpm = 0;
  s_stats.escaped = false;
}

void run_state_deinit(void) {
  // Nothing to clean up
}

bool run_state_start(void) {
  if (s_state == RUN_IDLE) {
    s_state = RUN_ACTIVE;
    s_stats.elapsed_seconds = 0;
    s_stats.distance_meters = 0;
    s_stats.avg_pace_sec_per_km = 0;
    s_stats.avg_hr_bpm = 0;
    s_stats.escaped = false;
    s_start_time = run_state_now();
    s_paused_elapsed = 0;
    return true;
  }
  return false;
}

bool run_state_pause(void) {
  if (s_state == RUN_ACTIVE) {
    s_paused_elapsed = (uint32_t)(run_state_now() - s_start_time);
    s_state = RUN_PAUSED;
    return true;
  }
  return false;
}

bool run_state_resume(void) {
  if (s_state == RUN_PAUSED) {
    s_start_time = run_state_now() - (time_t)s_paused_elapsed;
    s_state = RUN_ACTIVE;
    return true;
  }
  return false;
}

bool run_state_stop(void) {
  if (s_state == RUN_ACTIVE || s_state == RUN_PAUSED) {
    s_state = RUN_COMPLETE;
    return true;
  }
  return false;
}

void run_state_reset(void) {
  s_state = RUN_IDLE;
  s_stats.elapsed_seconds = 0;
  s_stats.distance_meters = 0;
  s_stats.avg_pace_sec_per_km = 0;
  s_stats.avg_hr_bpm = 0;
  s_stats.escaped = false;
}

RunState run_state_get(void) { return s_state; }

const RunStats *run_state_get_stats(void) { return &s_stats; }

void run_state_tick(void) {
  if (s_state == RUN_ACTIVE) {
    s_stats.elapsed_seconds = (uint32_t)(run_state_now() - s_start_time);

    if (s_stats.distance_meters > 0) {
      s_stats.avg_pace_sec_per_km =
          (s_stats.elapsed_seconds * 1000) / s_stats.distance_meters;
    }
  }
}

void run_state_add_distance(uint32_t meters) {
  s_stats.distance_meters += meters;
}

void run_state_set_distance(uint32_t meters) { s_stats.distance_meters = meters; }

void run_state_set_avg_hr(uint32_t bpm) { s_stats.avg_hr_bpm = bpm; }

void run_state_test_advance(uint32_t seconds) {
#ifdef RUN_STATE_HOST_TEST
  s_test_time_offset += (time_t)seconds;
#else
  (void)seconds;
#endif
}
