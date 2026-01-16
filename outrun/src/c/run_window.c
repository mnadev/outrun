/**
 * run_window.c - Main run UI window
 *
 * Horror-themed UI showing:
 * - Distance from Killer (large, centered)
 * - Current pace vs target pace
 * - Run state indicator
 */

#include "run_window.h"
#include "haptic_feedback.h"
#include "pace_engine.h"
#include "run_state.h"

// Window and layers
static Window *s_window;
static TextLayer *s_distance_label;
static TextLayer *s_distance_layer;
static TextLayer *s_pace_layer;
static TextLayer *s_status_layer;
static TextLayer *s_segment_layer;

// Timer for periodic updates
static AppTimer *s_update_timer;

// Text buffers
static char s_distance_buffer[16];
static char s_pace_buffer[32];
static char s_status_buffer[32];
static char s_segment_buffer[64];

// Previous state for haptic transitions
static PaceState s_prev_pace_state = PACE_ON_TARGET;

// Forward declarations
static void update_timer_callback(void *data);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_long_click_handler(ClickRecognizerRef recognizer,
                                      void *context);

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler,
                              NULL);
}

static void format_pace(char *buffer, size_t size, int32_t pace_sec_per_km) {
  int minutes = pace_sec_per_km / 60;
  int seconds = pace_sec_per_km % 60;
  snprintf(buffer, size, "%d:%02d", minutes, seconds);
}

void run_window_update(void) {
  const PaceData *pace = pace_engine_get_data();
  RunState state = run_state_get();

  // Update distance from killer
  snprintf(s_distance_buffer, sizeof(s_distance_buffer), "%ld",
           (long)pace->distance_from_killer);
  text_layer_set_text(s_distance_layer, s_distance_buffer);

  // Update pace display
  char current_pace[8], target_pace[8];
  format_pace(current_pace, sizeof(current_pace),
              pace->current_pace_sec_per_km);
  format_pace(target_pace, sizeof(target_pace), pace->target_pace_sec_per_km);
  snprintf(s_pace_buffer, sizeof(s_pace_buffer), "%s / %s", current_pace,
           target_pace);
  text_layer_set_text(s_pace_layer, s_pace_buffer);

  // Update status
  switch (state) {
  case RUN_IDLE:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "PRESS SELECT TO RUN");
    break;
  case RUN_ACTIVE:
    switch (pace->state) {
    case PACE_ON_TARGET:
      snprintf(s_status_buffer, sizeof(s_status_buffer), "SAFE... FOR NOW");
      break;
    case PACE_AHEAD:
      snprintf(s_status_buffer, sizeof(s_status_buffer), "OUTRUNNING!");
      break;
    case PACE_BEHIND:
      snprintf(s_status_buffer, sizeof(s_status_buffer), "IT'S GAINING!");
      break;
    case PACE_DANGER:
      snprintf(s_status_buffer, sizeof(s_status_buffer), "RUN FASTER!");
      break;
    }
    break;
  case RUN_PAUSED:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "HIDING...");
    break;
  case RUN_COMPLETE:
    if (pace->distance_from_killer > 0) {
      snprintf(s_status_buffer, sizeof(s_status_buffer), "ESCAPED!");
    } else {
      snprintf(s_status_buffer, sizeof(s_status_buffer), "CAUGHT!");
    }
    break;
  }
  text_layer_set_text(s_status_layer, s_status_buffer);

  // Handle haptic state transitions
  if (state == RUN_ACTIVE && pace->state != s_prev_pace_state) {
    switch (pace->state) {
    case PACE_ON_TARGET:
    case PACE_AHEAD:
      haptic_start_heartbeat(false);
      break;
    case PACE_BEHIND:
      haptic_start_heartbeat(true);
      break;
    case PACE_DANGER:
      haptic_pulse_danger();
      haptic_start_heartbeat(true);
      break;
    }
    s_prev_pace_state = pace->state;
  }
}

void run_window_show_segment_alert(const char *segment_name,
                                   const char *rival_name) {
  snprintf(s_segment_buffer, sizeof(s_segment_buffer),
           "SEGMENT: %s\nBeat %s or die!", segment_name, rival_name);
  text_layer_set_text(s_segment_layer, s_segment_buffer);
  layer_set_hidden(text_layer_get_layer(s_segment_layer), false);
  haptic_jump_scare();
}

void run_window_hide_segment_alert(void) {
  layer_set_hidden(text_layer_get_layer(s_segment_layer), true);
}

static void update_timer_callback(void *data) {
  RunState state = run_state_get();

  if (state == RUN_ACTIVE) {
    run_state_tick();

    // Simulate pace changes for testing (will be replaced by phone data)
    // In real use, pace comes from PebbleKit JS
    const PaceData *pace = pace_engine_get_data();
    pace_engine_update(pace->current_pace_sec_per_km);

    run_window_update();
  }

  // Re-schedule if running
  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    s_update_timer = app_timer_register(1000, update_timer_callback, NULL);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  RunState state = run_state_get();

  switch (state) {
  case RUN_IDLE:
    if (run_state_start()) {
      pace_engine_reset();
      haptic_start_heartbeat(false);
      s_update_timer = app_timer_register(1000, update_timer_callback, NULL);
    }
    break;
  case RUN_ACTIVE:
    if (run_state_pause()) {
      haptic_stop_heartbeat();
    }
    break;
  case RUN_PAUSED:
    if (run_state_resume()) {
      haptic_start_heartbeat(s_prev_pace_state == PACE_BEHIND ||
                             s_prev_pace_state == PACE_DANGER);
    }
    break;
  case RUN_COMPLETE:
    run_state_reset();
    break;
  }

  run_window_update();
}

static void select_long_click_handler(ClickRecognizerRef recognizer,
                                      void *context) {
  RunState state = run_state_get();

  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    run_state_stop();
    haptic_stop_heartbeat();

    // Final result haptic
    const PaceData *pace = pace_engine_get_data();
    if (pace->distance_from_killer > 0) {
      haptic_pulse_soft(); // Victory
    } else {
      haptic_jump_scare(); // Caught!
    }

    run_window_update();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Decrease target pace (faster)
  pace_engine_adjust_target(-15);
  run_window_update();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Increase target pace (slower)
  pace_engine_adjust_target(15);
  run_window_update();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Dark background
  window_set_background_color(window, GColorBlack);

  // "DISTANCE FROM KILLER" label
  s_distance_label = text_layer_create(GRect(0, 10, bounds.size.w, 20));
  text_layer_set_text(s_distance_label, "DISTANCE FROM KILLER");
  text_layer_set_font(s_distance_label,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_distance_label, GTextAlignmentCenter);
  text_layer_set_background_color(s_distance_label, GColorBlack);
  text_layer_set_text_color(s_distance_label, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_label));

  // Distance value (large)
  s_distance_layer = text_layer_create(GRect(0, 30, bounds.size.w, 50));
  text_layer_set_text(s_distance_layer, "100");
  text_layer_set_font(s_distance_layer,
                      fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_distance_layer, GColorBlack);
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));

  // Pace display (current / target)
  s_pace_layer = text_layer_create(GRect(0, 85, bounds.size.w, 24));
  text_layer_set_text(s_pace_layer, "0:00 / 5:00");
  text_layer_set_font(s_pace_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_pace_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_pace_layer, GColorBlack);
  text_layer_set_text_color(s_pace_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_pace_layer));

  // Status message
  s_status_layer = text_layer_create(GRect(0, 115, bounds.size.w, 30));
  text_layer_set_text(s_status_layer, "PRESS SELECT TO RUN");
  text_layer_set_font(s_status_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_status_layer, GColorBlack);
  text_layer_set_text_color(s_status_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  // Segment alert (hidden by default)
  s_segment_layer = text_layer_create(GRect(0, 145, bounds.size.w, 40));
  text_layer_set_font(s_segment_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_segment_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_segment_layer, GColorWhite);
  text_layer_set_text_color(s_segment_layer, GColorBlack);
  layer_set_hidden(text_layer_get_layer(s_segment_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_segment_layer));

  run_window_update();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_distance_label);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_pace_layer);
  text_layer_destroy(s_status_layer);
  text_layer_destroy(s_segment_layer);

  if (s_update_timer) {
    app_timer_cancel(s_update_timer);
    s_update_timer = NULL;
  }

  window_destroy(s_window);
}

void run_window_push(void) {
  s_window = window_create();

  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(
      s_window, (WindowHandlers){.load = window_load, .unload = window_unload});

  window_stack_push(s_window, true);
}
