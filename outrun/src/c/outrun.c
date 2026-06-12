/**
 * outrun.c - Main entry point for Outrun running pacer watchapp
 */

#include "comm.h"
#include "features.h"
#include "hr_monitor.h"
#include "haptic_feedback.h"
#include "menu_window.h"
#include "pace_engine.h"
#include "plan.h"
#include "plans_window.h"
#include "run_session.h"
#include "run_state.h"
#include "settings.h"
#include <pebble.h>

static void on_plans_received(void) { plans_window_reload(); }

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun pacer initializing...");

  settings_init();
  features_init();
  pace_engine_init(settings_get()->target_pace_sec_per_km);
  haptic_init();
  hr_monitor_init();
  run_state_init();
  run_session_init();
  plan_store_init();
  plan_store_load_defaults();
  comm_init();
  comm_set_plan_received_callback(on_plans_received);

  menu_window_push();

  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun pacer ready.");
}

static void deinit(void) {
  comm_deinit();
  hr_monitor_deinit();
  haptic_deinit();
  run_state_deinit();
  APP_LOG(APP_LOG_LEVEL_INFO, "Outrun shutting down.");
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
