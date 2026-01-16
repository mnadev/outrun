/**
 * run_state.c - Run state machine implementation
 */

#include "run_state.h"

static RunState s_state = RUN_IDLE;
static RunStats s_stats;

void run_state_init(void) {
  s_state = RUN_IDLE;
  s_stats.elapsed_seconds = 0;
  s_stats.distance_meters = 0;
  s_stats.avg_pace_sec_per_km = 0;
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
    s_stats.escaped = false;
    return true;
  }
  return false;
}

bool run_state_pause(void) {
  if (s_state == RUN_ACTIVE) {
    s_state = RUN_PAUSED;
    return true;
  }
  return false;
}

bool run_state_resume(void) {
  if (s_state == RUN_PAUSED) {
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
  s_stats.escaped = false;
}

RunState run_state_get(void) { return s_state; }

const RunStats *run_state_get_stats(void) { return &s_stats; }

void run_state_tick(void) {
  if (s_state == RUN_ACTIVE) {
    s_stats.elapsed_seconds++;

    // Recalculate average pace
    if (s_stats.distance_meters > 0) {
      // pace = seconds per km = (elapsed_sec * 1000) / distance_m
      s_stats.avg_pace_sec_per_km =
          (s_stats.elapsed_seconds * 1000) / s_stats.distance_meters;
    }
  }
}

void run_state_add_distance(uint32_t meters) {
  s_stats.distance_meters += meters;
}
