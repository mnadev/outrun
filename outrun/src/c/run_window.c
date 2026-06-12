/**
 * run_window.c - Running pacer UI
 */

#include "run_window.h"
#include "comm.h"
#include "haptic_feedback.h"
#include "hr_monitor.h"
#include "pace_engine.h"
#include "plan.h"
#include "run_session.h"
#include "run_state.h"
#include "settings.h"

static Window *s_window;
static TextLayer *s_pace_label;
static TextLayer *s_pace_layer;
static TextLayer *s_hr_layer;
static TextLayer *s_stats_layer;
static TextLayer *s_segment_layer;
static TextLayer *s_status_layer;
static AppTimer *s_update_timer;
static bool s_show_summary;

static char s_pace_buffer[32];
static char s_hr_buffer[24];
static char s_stats_buffer[40];
static char s_segment_buffer[40];
static char s_status_buffer[32];

static void update_timer_callback(void *data);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, NULL);
}

static void format_elapsed(char *buffer, size_t size, uint32_t seconds) {
  uint32_t mins = seconds / 60;
  uint32_t secs = seconds % 60;
  snprintf(buffer, size, "%lu:%02lu", (unsigned long)mins, (unsigned long)secs);
}

void run_window_update(void) {
  const PaceData *pace = pace_engine_get_data();
  const RunStats *stats = run_state_get_stats();
  const AppSettings *settings = settings_get();
  RunState state = run_state_get();

  if (s_show_summary && state == RUN_COMPLETE) {
    char elapsed[12];
    char pace_str[12];
    char dist[16];
    format_elapsed(elapsed, sizeof(elapsed), stats->elapsed_seconds);
    settings_format_pace(pace_str, sizeof(pace_str), stats->avg_pace_sec_per_km);
    settings_format_distance(dist, sizeof(dist), stats->distance_meters);

    snprintf(s_pace_buffer, sizeof(s_pace_buffer), "DONE");
    text_layer_set_text(s_pace_label, "Summary");
    text_layer_set_text(s_pace_layer, s_pace_buffer);

    if (hr_monitor_is_available() && stats->avg_hr_bpm > 0) {
      snprintf(s_hr_buffer, sizeof(s_hr_buffer), "Avg HR %lu",
               (unsigned long)stats->avg_hr_bpm);
    } else {
      snprintf(s_hr_buffer, sizeof(s_hr_buffer), " ");
    }
    text_layer_set_text(s_hr_layer, s_hr_buffer);

    snprintf(s_stats_buffer, sizeof(s_stats_buffer), "%s  %s\n%s", elapsed, dist, pace_str);
    text_layer_set_text(s_stats_layer, s_stats_buffer);
    text_layer_set_text(s_segment_layer, "");
    text_layer_set_text(s_status_layer, "SELECT to exit");
    return;
  }

  char current[8], target[8];
  settings_format_pace(current, sizeof(current), pace->current_pace_sec_per_km);
  settings_format_pace(target, sizeof(target), pace->target_pace_sec_per_km);
  snprintf(s_pace_buffer, sizeof(s_pace_buffer), "%s / %s", current, target);
  text_layer_set_text(s_pace_layer, s_pace_buffer);

  if (hr_monitor_is_available()) {
    uint8_t bpm = hr_monitor_get_bpm();
    snprintf(s_hr_buffer, sizeof(s_hr_buffer), "HR %u (%u-%u)", bpm,
             settings->hr_zone_lo, settings->hr_zone_hi);
    layer_set_hidden(text_layer_get_layer(s_hr_layer), false);
  } else {
    snprintf(s_hr_buffer, sizeof(s_hr_buffer), " ");
    layer_set_hidden(text_layer_get_layer(s_hr_layer), true);
  }
  text_layer_set_text(s_hr_layer, s_hr_buffer);

  char elapsed[12];
  char dist[16];
  format_elapsed(elapsed, sizeof(elapsed), stats->elapsed_seconds);
  settings_format_distance(dist, sizeof(dist), stats->distance_meters);
  snprintf(s_stats_buffer, sizeof(s_stats_buffer), "%s  %s", elapsed, dist);
  text_layer_set_text(s_stats_layer, s_stats_buffer);

  if (run_session_is_planned()) {
    const PlanProgress *progress = run_session_get_progress();
    const PlanSegment *segment = plan_progress_current_segment(progress);
    if (segment) {
      uint32_t remaining = plan_progress_remaining(progress);
      if (segment->end_type == SEG_END_DURATION) {
        snprintf(s_segment_buffer, sizeof(s_segment_buffer), "%s  %lus left",
                 plan_segment_label_name(segment->label), (unsigned long)remaining);
      } else {
        snprintf(s_segment_buffer, sizeof(s_segment_buffer), "%s  %lum left",
                 plan_segment_label_name(segment->label), (unsigned long)remaining);
      }
      text_layer_set_text(s_segment_layer, s_segment_buffer);
    }
  } else {
    text_layer_set_text(s_segment_layer, "");
  }

  switch (state) {
  case RUN_IDLE:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "SELECT to start");
    break;
  case RUN_ACTIVE:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "Running");
    break;
  case RUN_PAUSED:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "Paused");
    break;
  case RUN_COMPLETE:
    snprintf(s_status_buffer, sizeof(s_status_buffer), "Complete");
    break;
  }
  text_layer_set_text(s_status_layer, s_status_buffer);
}

void run_window_show_segment_alert(const char *segment_name, const char *rival_name) {
  (void)segment_name;
  (void)rival_name;
}

void run_window_hide_segment_alert(void) {}

static void schedule_timer(void) {
  if (s_update_timer) {
    app_timer_cancel(s_update_timer);
  }
  RunState state = run_state_get();
  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    s_update_timer = app_timer_register(1000, update_timer_callback, NULL);
  }
}

static void update_timer_callback(void *data) {
  RunState state = run_state_get();
  if (state == RUN_ACTIVE) {
    run_state_tick();
    run_session_tick();
    run_window_update();
  }
  schedule_timer();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  RunState state = run_state_get();

  switch (state) {
  case RUN_IDLE:
    if (run_session_get_type() == SESSION_PLANNED) {
      run_session_start_planned(run_session_pending_plan_index());
    } else {
      run_session_start_quick();
    }
    s_show_summary = false;
    schedule_timer();
    break;
  case RUN_ACTIVE:
    run_state_pause();
    comm_send_command(CMD_PAUSE);
    break;
  case RUN_PAUSED:
    run_state_resume();
    comm_send_command(CMD_RESUME);
    schedule_timer();
    break;
  case RUN_COMPLETE:
    run_state_reset();
    run_session_init();
    s_show_summary = false;
    window_stack_pop(true);
    return;
  default:
    break;
  }

  run_window_update();
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  RunState state = run_state_get();
  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    run_state_stop();
    run_session_stop();
    s_show_summary = true;
    if (s_update_timer) {
      app_timer_cancel(s_update_timer);
      s_update_timer = NULL;
    }
    run_window_update();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (run_session_is_planned() || run_state_get() == RUN_COMPLETE) {
    return;
  }
  pace_engine_adjust_target(-15);
  const PaceData *pace = pace_engine_get_data();
  comm_send_target_pace(pace->target_pace_sec_per_km);
  run_window_update();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (run_session_is_planned() || run_state_get() == RUN_COMPLETE) {
    return;
  }
  pace_engine_adjust_target(15);
  const PaceData *pace = pace_engine_get_data();
  comm_send_target_pace(pace->target_pace_sec_per_km);
  run_window_update();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  int top = PBL_IF_ROUND_ELSE(28, 10);
  int pace_y = PBL_IF_ROUND_ELSE(48, 30);
  int hr_y = PBL_IF_ROUND_ELSE(78, 58);
  int stats_y = PBL_IF_ROUND_ELSE(100, 82);
  int seg_y = PBL_IF_ROUND_ELSE(122, 104);
  int status_y = PBL_IF_ROUND_ELSE(144, 126);

  s_pace_label = text_layer_create(GRect(0, top, bounds.size.w, 18));
  text_layer_set_text(s_pace_label, "Pace");
  text_layer_set_font(s_pace_label, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_pace_label, GTextAlignmentCenter);
  text_layer_set_background_color(s_pace_label, GColorBlack);
  text_layer_set_text_color(s_pace_label, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_pace_label));

  s_pace_layer = text_layer_create(GRect(0, pace_y, bounds.size.w, 28));
  text_layer_set_text(s_pace_layer, "0:00 / 5:00");
  text_layer_set_font(s_pace_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_pace_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_pace_layer, GColorBlack);
  text_layer_set_text_color(s_pace_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_pace_layer));

  s_hr_layer = text_layer_create(GRect(0, hr_y, bounds.size.w, 20));
  text_layer_set_font(s_hr_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_hr_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_hr_layer, GColorBlack);
  text_layer_set_text_color(s_hr_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_hr_layer));

  s_stats_layer = text_layer_create(GRect(0, stats_y, bounds.size.w, 20));
  text_layer_set_font(s_stats_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_stats_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_stats_layer, GColorBlack);
  text_layer_set_text_color(s_stats_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_stats_layer));

  s_segment_layer = text_layer_create(GRect(0, seg_y, bounds.size.w, 20));
  text_layer_set_font(s_segment_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_segment_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_segment_layer, GColorBlack);
  text_layer_set_text_color(s_segment_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_segment_layer));

  s_status_layer = text_layer_create(GRect(0, status_y, bounds.size.w, 20));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_status_layer, GColorBlack);
  text_layer_set_text_color(s_status_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  run_window_update();
}

static void window_unload(Window *window) {
  if (s_update_timer) {
    app_timer_cancel(s_update_timer);
    s_update_timer = NULL;
  }

  text_layer_destroy(s_pace_label);
  text_layer_destroy(s_pace_layer);
  text_layer_destroy(s_hr_layer);
  text_layer_destroy(s_stats_layer);
  text_layer_destroy(s_segment_layer);
  text_layer_destroy(s_status_layer);
  window_destroy(s_window);
  s_window = NULL;
}

static void push_window(void) {
  s_window = window_create();
  s_show_summary = false;
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window,
                             (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
}

void run_window_push_quick(void) {
  run_session_init();
  push_window();
}

void run_window_push_planned(uint8_t plan_index) {
  run_session_init();
  run_session_prepare_planned(plan_index);
  push_window();
  if (run_session_start_planned(plan_index)) {
    s_show_summary = false;
    schedule_timer();
    run_window_update();
  }
}
