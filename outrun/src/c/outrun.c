/**
 * outrun.c - Main entry point for Outrun watchapp
 *
 * "The Social Horror Pacer" - A gamified survival fitness app
 * that turns runs into slasher-movie escapes.
 */

#include "comm.h"
#include "features.h"
#include "haptic_feedback.h"
#include "pace_engine.h"
#include "run_state.h"
#include "run_window.h"
#include <pebble.h>

// Default target pace: 5:00 per km (300 seconds)
#define DEFAULT_TARGET_PACE_SEC 300

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun initializing...");

  // Initialize subsystems
  features_init();
  pace_engine_init(DEFAULT_TARGET_PACE_SEC);
  haptic_init();
  run_state_init();
  comm_init(); // Initialize phone communication

  // Push the main run window
  run_window_push();

  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun ready. Waiting for phone connection...");
}

static void deinit(void) {
  comm_deinit();
  haptic_deinit();
  run_state_deinit();

  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun shutting down.");
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
