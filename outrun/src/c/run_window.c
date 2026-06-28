/**
 * run_window.c - Running pacer UI
 */

#include "run_window.h"
#include "accent.h"
#include "comm.h"
#include "haptic_feedback.h"
#include "pace_engine.h"
#include "plan.h"
#include "run_session.h"
#include "run_state.h"
#include "settings.h"
#include "stalker_themes.h"
#include "watch_interface.h"

static Window *s_window;
static TextLayer *s_pace_label;
static TextLayer *s_pace_layer;
static TextLayer *s_hr_layer;
static TextLayer *s_stats_layer;
static TextLayer *s_segment_layer;
static TextLayer *s_status_layer;
static Layer *s_killer_bar;
static bool s_show_summary;
static bool s_tick_subscribed;

typedef enum { HB_OFF, HB_SOFT, HB_RAPID } HbMode;
static HbMode s_hb_mode;

static bool s_rival_active;
static char s_rival_buffer[40];

// Live-pace honesty: don't present the target as if it were a live reading
// before the phone delivers real GPS pace, or after the feed goes stale.
#define PACE_STALE_SEC 10        // no pace for this long -> treat as lost
#define GPS_ACQUIRE_GRACE_SEC 30 // after this with no pace ever -> "no signal"
static bool s_live_pace;         // have we ever received pace this run?
static uint32_t s_last_pace_sec; // elapsed seconds at the last pace update

static char s_pace_buffer[32];
static char s_hr_buffer[24];
static char s_stats_buffer[40];
static char s_segment_buffer[40];

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void back_click_handler(ClickRecognizerRef recognizer, void *context);
static void stop_run_with_summary(void);

static void subscribe_tick(void) {
  if (!s_tick_subscribed) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    s_tick_subscribed = true;
  }
}

static void unsubscribe_tick(void) {
  if (s_tick_subscribed) {
    tick_timer_service_unsubscribe();
    s_tick_subscribed = false;
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, NULL);
}

static void format_elapsed(char *buffer, size_t size, uint32_t seconds) {
  uint32_t hours = seconds / 3600;
  uint32_t mins = (seconds % 3600) / 60;
  uint32_t secs = seconds % 60;
  if (hours > 0) {
    snprintf(buffer, size, "%lu:%02lu:%02lu", (unsigned long)hours,
             (unsigned long)mins, (unsigned long)secs);
  } else {
    snprintf(buffer, size, "%lu:%02lu", (unsigned long)mins, (unsigned long)secs);
  }
}

// Traffic-light feedback by pace state; white on black-and-white watches.
static GColor pace_state_color(PaceState st) {
#if defined(PBL_COLOR)
  switch (st) {
  case PACE_AHEAD:
    return GColorGreen;
  case PACE_BEHIND:
    return GColorYellow;
  case PACE_DANGER:
    return GColorRed;
  default:
    return GColorWhite;
  }
#else
  (void)st;
  return GColorWhite;
#endif
}

// Killer bar uses the active stalker's colors: its primary hue while you're
// safe, yellow when behind, its danger hue when you're about to be caught.
static GColor killer_bar_color(PaceState st) {
#if defined(PBL_COLOR)
  switch (st) {
  case PACE_DANGER:
    return themes_get_danger_color(themes_get_current());
  case PACE_BEHIND:
    return GColorYellow;
  default:
    return accent_get_color();
  }
#else
  (void)st;
  return GColorWhite;
#endif
}

// "Distance from killer": a lead bar that empties as the stalker closes in.
static void killer_bar_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  const PaceData *pace = pace_engine_get_data();

  int32_t dist = pace->distance_from_killer;
  if (dist < 0) {
    dist = 0;
  }
  if (dist > 200) {
    dist = 200;
  }
  int fill_w = (b.size.w * (int)dist) / 200;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(0, 0, b.size.w, b.size.h));

  graphics_context_set_fill_color(ctx, killer_bar_color(pace->state));
  if (fill_w > 2) {
    graphics_fill_rect(ctx, GRect(1, 1, fill_w - 2, b.size.h - 2), 0, GCornerNone);
  }
}

static TextLayer *make_run_label(Layer *parent, GRect frame, const char *font_key) {
  TextLayer *t = text_layer_create(frame);
  text_layer_set_font(t, fonts_get_system_font(font_key));
  text_layer_set_text_alignment(t, GTextAlignmentCenter);
  text_layer_set_background_color(t, GColorClear);
  text_layer_set_text_color(t, GColorWhite);
  layer_add_child(parent, text_layer_get_layer(t));
  return t;
}

// The chase heartbeat: silent when safe, the stalker's pulse when you fall
// behind, its frantic danger beat when you're about to be caught. Re-armed
// only when the level changes so the timer never thrashes.
static void update_heartbeat(RunState state, PaceState ps) {
  HbMode want = HB_OFF;
  if (state == RUN_ACTIVE) {
    if (ps == PACE_DANGER) {
      want = HB_RAPID;
    } else if (ps == PACE_BEHIND) {
      want = HB_SOFT;
    }
  }

  if (want == s_hb_mode) {
    return;
  }
  s_hb_mode = want;

  if (want == HB_OFF) {
    haptic_stop_heartbeat();
  } else {
    haptic_start_heartbeat(want == HB_RAPID);
  }
}

void run_window_show_summary(void) {
  s_show_summary = true;
  unsubscribe_tick();
  run_window_update();
}

void run_window_update(void) {
  if (!s_window || !s_pace_layer) {
    return;
  }

  const PaceData *pace = pace_engine_get_data();
  const RunStats *stats = run_state_get_stats();
  const AppSettings *settings = settings_get();
  RunState state = run_state_get();

  update_heartbeat(state, pace->state);

  if (s_show_summary && state == RUN_COMPLETE) {
    const ThemeConfig *theme = themes_get_current_config();
    char pace_str[12];
    layer_set_hidden(s_killer_bar, true);
    int32_t target = pace_engine_get_data()->target_pace_sec_per_km;
    uint32_t escape_threshold = (uint32_t)(target + PACE_TOLERANCE_DANGER);

    // Verdict: did we keep ahead of the killer? (neutral when no GPS data)
    bool has_pace = stats->avg_pace_sec_per_km > 0;
    bool escaped = has_pace && stats->avg_pace_sec_per_km <= escape_threshold;

    const char *verdict = "Summary";
    if (has_pace) {
      verdict = escaped ? theme->escape_message : theme->caught_message;
    }
    text_layer_set_text(s_pace_label, verdict);
#if defined(PBL_COLOR)
    GColor verdict_color =
        !has_pace ? GColorWhite : (escaped ? GColorGreen : GColorRed);
    text_layer_set_text_color(s_pace_label, verdict_color);
#endif

    // Hero stat: elapsed time.
    format_elapsed(s_pace_buffer, sizeof(s_pace_buffer), stats->elapsed_seconds);
    text_layer_set_text(s_pace_layer, s_pace_buffer);
    text_layer_set_text_color(s_pace_layer, GColorWhite);

    // Distance, then average pace, then average HR -- one per line.
    settings_format_distance(s_hr_buffer, sizeof(s_hr_buffer),
                             stats->distance_meters);
    layer_set_hidden(text_layer_get_layer(s_hr_layer), false);
    text_layer_set_text(s_hr_layer, s_hr_buffer);

    settings_format_pace(pace_str, sizeof(pace_str), stats->avg_pace_sec_per_km);
    // settings_format_pace already converts to the display unit; label it to
    // match (per mile in miles mode, not a hardcoded "/km").
    const char *pace_unit = (settings->units == UNITS_MI) ? "/mi" : "/km";
    snprintf(s_stats_buffer, sizeof(s_stats_buffer), "%s %s", pace_str, pace_unit);
    text_layer_set_text(s_stats_layer, s_stats_buffer);

    if (stats->avg_hr_bpm > 0) {
      snprintf(s_segment_buffer, sizeof(s_segment_buffer), "Avg HR %lu",
               (unsigned long)stats->avg_hr_bpm);
    } else {
      s_segment_buffer[0] = '\0';
    }
    text_layer_set_text(s_segment_layer, s_segment_buffer);

    text_layer_set_text(s_status_layer, "SELECT to exit");
    return;
  }

  layer_set_hidden(s_killer_bar, false);
  layer_mark_dirty(s_killer_bar);

  // Live pace is "fresh" only if we've received one this run and it's recent.
  bool pace_fresh = s_live_pace &&
                    (stats->elapsed_seconds - s_last_pace_sec <= PACE_STALE_SEC);

  char current[8], target[8];
  if (pace_fresh) {
    settings_format_pace(current, sizeof(current), pace->current_pace_sec_per_km);
  } else {
    snprintf(current, sizeof(current), "--:--");
  }
  settings_format_pace(target, sizeof(target), pace->target_pace_sec_per_km);
  snprintf(s_pace_buffer, sizeof(s_pace_buffer), "%s / %s", current, target);
  text_layer_set_text(s_pace_layer, s_pace_buffer);
  text_layer_set_text_color(s_pace_layer,
                            pace_fresh ? pace_state_color(pace->state) : GColorWhite);

  if (watch_heart_rate_available()) {
    uint8_t bpm = watch_heart_rate();
    uint8_t hr_lo = settings->hr_zone_lo;
    uint8_t hr_hi = settings->hr_zone_hi;

    if (run_session_is_planned()) {
      const PlanProgress *progress = run_session_get_progress();
      const PlanSegment *segment = plan_progress_current_segment(progress);
      if (segment && segment->target_type == SEG_TARGET_HR) {
        hr_lo = (uint8_t)segment->target_lo;
        hr_hi = (uint8_t)segment->target_hi;
      }
    }

    if (bpm > 0) {
      snprintf(s_hr_buffer, sizeof(s_hr_buffer), "HR %u (%u-%u)", bpm, hr_lo, hr_hi);
      layer_set_hidden(text_layer_get_layer(s_hr_layer), false);
#if defined(PBL_COLOR)
      GColor hr_color = GColorWhite;
      if (bpm > hr_hi) {
        hr_color = GColorRed; // working too hard
      } else if (bpm < hr_lo) {
        hr_color = GColorCyan; // below target zone
      }
      text_layer_set_text_color(s_hr_layer, hr_color);
#endif
    } else {
      snprintf(s_hr_buffer, sizeof(s_hr_buffer), " ");
      layer_set_hidden(text_layer_get_layer(s_hr_layer), true);
    }
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

  if (s_rival_active) {
    // A rival (ghost/segment) takes over the segment line.
    text_layer_set_text(s_segment_layer, s_rival_buffer);
#if defined(PBL_COLOR)
    text_layer_set_text_color(s_segment_layer, accent_get_color());
#endif
  } else if (run_session_is_planned()) {
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
#if defined(PBL_COLOR)
      text_layer_set_text_color(s_segment_layer, GColorWhite);
#endif
    }
  } else {
    text_layer_set_text(s_segment_layer, "");
  }

  const ThemeConfig *theme = themes_get_current_config();
  const char *status = "";
#if defined(PBL_COLOR)
  GColor status_color = GColorWhite;
#endif
  switch (state) {
  case RUN_IDLE:
    status = "SELECT to start";
    break;
  case RUN_ACTIVE:
    if (!pace_fresh) {
      // No live GPS pace: say so honestly instead of voicing a faked chase.
      if (!s_live_pace) {
        status = (stats->elapsed_seconds < GPS_ACQUIRE_GRACE_SEC)
                     ? "Acquiring GPS"
                     : "No GPS signal";
      } else {
        status = "GPS lost";
      }
      break;
    }
    // Speak in the theme's voice based on how the chase is going.
    switch (pace->state) {
    case PACE_AHEAD:
      status = theme->ahead_message; // e.g. "OUTRUNNING!"
#if defined(PBL_COLOR)
      status_color = GColorGreen;
#endif
      break;
    case PACE_BEHIND:
      status = theme->behind_message; // e.g. "IT'S GAINING!"
#if defined(PBL_COLOR)
      status_color = GColorYellow;
#endif
      break;
    case PACE_DANGER:
      status = theme->behind_message;
#if defined(PBL_COLOR)
      status_color = GColorRed;
#endif
      break;
    default:
      status = "On pace";
      break;
    }
    break;
  case RUN_PAUSED:
    status = run_session_is_auto_paused() ? "Auto-paused" : "Paused";
    break;
  case RUN_COMPLETE:
    status = "Complete";
    break;
  }
  text_layer_set_text(s_status_layer, status);
#if defined(PBL_COLOR)
  text_layer_set_text_color(s_status_layer, status_color);
#endif
}

void run_window_note_live_pace(void) {
  s_live_pace = true;
  s_last_pace_sec = run_state_get_stats()->elapsed_seconds;
  run_window_update();
}

void run_window_show_segment_alert(const char *segment_name, const char *rival_name) {
  (void)segment_name;
  bool was_active = s_rival_active;
  s_rival_active = true;
  snprintf(s_rival_buffer, sizeof(s_rival_buffer), "vs %s",
           rival_name ? rival_name : "Rival");
  // Jump scare only when the rival first appears AND the run window is live
  // with an active/paused run. Otherwise a segment/ghost message that arrives
  // while you're in another window (or not running) would buzz out of nowhere.
  if (!was_active && s_window && run_session_is_active()) {
    haptic_jump_scare();
  }
  run_window_update();
}

void run_window_hide_segment_alert(void) {
  s_rival_active = false;
  run_window_update();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (!(units_changed & SECOND_UNIT)) {
    return;
  }

  RunState before = run_state_get();
  if (before == RUN_ACTIVE) {
    run_state_tick();
    run_session_tick();

    if (run_state_get() == RUN_COMPLETE) {
      run_window_show_summary();
      return;
    }
    run_window_update();
  }
}

static void stop_run_with_summary(void) {
  run_state_stop();
  run_session_stop();
  run_window_show_summary();
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
    subscribe_tick();
    break;
  case RUN_ACTIVE:
    run_state_pause();
    watch_heart_rate_stop();
    run_session_reset_auto_pause(); // manual pause overrides auto-pause
    comm_send_command(CMD_PAUSE);
    break;
  case RUN_PAUSED:
    run_state_resume();
    watch_heart_rate_start();
    run_session_reset_auto_pause(); // manual resume clears any auto-pause
    comm_send_command(CMD_RESUME);
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
    stop_run_with_summary();
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  RunState state = run_state_get();

  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    stop_run_with_summary();
    return;
  }

  if (state == RUN_COMPLETE) {
    run_state_reset();
    run_session_init();
    s_show_summary = false;
  }

  window_stack_pop(true);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (run_session_is_planned() || run_state_get() == RUN_COMPLETE) {
    return;
  }
  pace_engine_adjust_target(
      settings_pace_step_to_sec_per_km(-PACE_ADJUST_STEP_SEC));
  const PaceData *pace = pace_engine_get_data();
  comm_send_target_pace(pace->target_pace_sec_per_km);
  run_window_update();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (run_session_is_planned() || run_state_get() == RUN_COMPLETE) {
    return;
  }
  pace_engine_adjust_target(
      settings_pace_step_to_sec_per_km(PACE_ADJUST_STEP_SEC));
  const PaceData *pace = pace_engine_get_data();
  comm_send_target_pace(pace->target_pace_sec_per_km);
  run_window_update();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  // Wider side inset on round watches; the stack is centered vertically so it
  // adapts to taller screens (emery) instead of clustering near the top.
  const int side = PBL_IF_ROUND_ELSE(30, 4);
  const int cw = bounds.size.w - 2 * side;

  const int h_label = 22;
  const int h_pace = 32;
  const int h_bar = 12;
  const int h_stats = 20;
  const int h_hr = 20;
  const int h_seg = 18;
  const int h_status = 20;
  const int gap = 2;
  const int total =
      h_label + h_pace + h_bar + h_stats + h_hr + h_seg + h_status + gap * 6;

  int y = (bounds.size.h - total) / 2;
  if (y < 0) {
    y = 0;
  }

  s_pace_label = make_run_label(window_layer, GRect(side, y, cw, h_label),
                                FONT_KEY_GOTHIC_18_BOLD);
  text_layer_set_text(s_pace_label, themes_get_current_config()->stalker_name);
  y += h_label + gap;

  s_pace_layer = make_run_label(window_layer, GRect(side, y, cw, h_pace),
                                FONT_KEY_GOTHIC_24_BOLD);
  text_layer_set_text(s_pace_layer, "0:00 / 5:00");
  y += h_pace + gap;

  s_killer_bar = layer_create(GRect(side, y, cw, h_bar));
  layer_set_update_proc(s_killer_bar, killer_bar_update);
  layer_add_child(window_layer, s_killer_bar);
  y += h_bar + gap;

  s_stats_layer = make_run_label(window_layer, GRect(side, y, cw, h_stats),
                                 FONT_KEY_GOTHIC_14);
  y += h_stats + gap;

  s_hr_layer = make_run_label(window_layer, GRect(side, y, cw, h_hr),
                              FONT_KEY_GOTHIC_14);
  y += h_hr + gap;

  s_segment_layer = make_run_label(window_layer, GRect(side, y, cw, h_seg),
                                   FONT_KEY_GOTHIC_14_BOLD);
  y += h_seg + gap;

  s_status_layer = make_run_label(window_layer, GRect(side, y, cw, h_status),
                                  FONT_KEY_GOTHIC_14);

  run_window_update();

  RunState state = run_state_get();
  if (state == RUN_ACTIVE || state == RUN_PAUSED) {
    subscribe_tick();
  }
}

static void window_unload(Window *window) {
  unsubscribe_tick();
  haptic_stop_heartbeat();
  s_hb_mode = HB_OFF;

  text_layer_destroy(s_pace_label);
  text_layer_destroy(s_pace_layer);
  text_layer_destroy(s_hr_layer);
  text_layer_destroy(s_stats_layer);
  text_layer_destroy(s_segment_layer);
  text_layer_destroy(s_status_layer);
  layer_destroy(s_killer_bar);
  s_pace_label = NULL;
  s_pace_layer = NULL;
  s_hr_layer = NULL;
  s_stats_layer = NULL;
  s_segment_layer = NULL;
  s_status_layer = NULL;
  s_killer_bar = NULL;
  window_destroy(s_window);
  s_window = NULL;
}

static void push_window(void) {
  s_window = window_create();
  s_show_summary = false;
  s_hb_mode = HB_OFF;
  s_rival_active = false;
  s_live_pace = false;
  s_last_pace_sec = 0;
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window,
                             (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
}

void run_window_push_quick(void) {
  if (run_state_get() == RUN_IDLE && run_session_get_type() == SESSION_NONE) {
    run_session_init();
    s_show_summary = false;
  }
  push_window();
}

void run_window_push_planned(uint8_t plan_index) {
  if (run_state_get() == RUN_IDLE && run_session_get_type() == SESSION_NONE) {
    run_session_init();
  }
  run_session_prepare_planned(plan_index);
  push_window();
  if (run_session_start_planned(plan_index)) {
    s_show_summary = false;
    subscribe_tick();
    run_window_update();
  }
}
