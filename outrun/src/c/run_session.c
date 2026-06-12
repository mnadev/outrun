/**
 * run_session.c - Coordinates quick runs and planned runs
 */

#include "run_session.h"
#include "comm.h"
#include "haptic_feedback.h"
#include "hr_monitor.h"
#include "pace_engine.h"
#include "run_state.h"
#include "settings.h"

#include <pebble.h>

static SessionType s_session_type;
static uint8_t s_pending_plan_index;
static PlanProgress s_plan_progress;
static AlertState s_alert_state;
static uint32_t s_segment_start_elapsed;
static uint32_t s_segment_start_distance;
static uint8_t s_prev_segment_index;

static void apply_segment_targets(const PlanSegment *segment) {
  if (!segment) {
    return;
  }

  if (segment->target_type == SEG_TARGET_PACE) {
    int32_t mid = ((int32_t)segment->target_lo + (int32_t)segment->target_hi) / 2;
    pace_engine_init(mid);
  }
}

static void build_alert_input(AlertInput *input, int32_t current_pace) {
  const AppSettings *settings = settings_get();
  const PlanSegment *segment = plan_progress_current_segment(&s_plan_progress);

  input->pace_alerts_enabled = settings->pace_alerts_enabled;
  input->hr_alerts_enabled = settings->hr_alerts_enabled;
  input->current_pace_sec_per_km = current_pace;
  input->current_hr = hr_monitor_get_bpm();
  input->hr_available = hr_monitor_is_available();

  if (s_session_type == SESSION_PLANNED && segment) {
    if (segment->target_type == SEG_TARGET_PACE) {
      input->pace_lo_sec_per_km = (int32_t)segment->target_lo;
      input->pace_hi_sec_per_km = (int32_t)segment->target_hi;
      input->pace_alerts_enabled = true;
    } else {
      input->pace_lo_sec_per_km = 0;
      input->pace_hi_sec_per_km = 9999;
      input->pace_alerts_enabled = false;
    }

    if (segment->target_type == SEG_TARGET_HR) {
      input->hr_lo = (uint8_t)segment->target_lo;
      input->hr_hi = (uint8_t)segment->target_hi;
      input->hr_alerts_enabled = true;
    } else {
      input->hr_lo = settings->hr_zone_lo;
      input->hr_hi = settings->hr_zone_hi;
      input->hr_alerts_enabled = settings->hr_alerts_enabled;
    }
  } else {
    int32_t target = settings->target_pace_sec_per_km;
    input->pace_lo_sec_per_km = target - PACE_TOLERANCE_SOFT;
    input->pace_hi_sec_per_km = target + PACE_TOLERANCE_SOFT;
    input->hr_lo = settings->hr_zone_lo;
    input->hr_hi = settings->hr_zone_hi;
  }
}

void run_session_init(void) {
  s_session_type = SESSION_NONE;
  s_pending_plan_index = 0;
  plan_progress_init(&s_plan_progress);
  alert_engine_init(&s_alert_state);
  s_segment_start_elapsed = 0;
  s_segment_start_distance = 0;
  s_prev_segment_index = 0;
}

void run_session_prepare_planned(uint8_t plan_index) {
  s_session_type = SESSION_PLANNED;
  s_pending_plan_index = plan_index;
}

uint8_t run_session_pending_plan_index(void) { return s_pending_plan_index; }

SessionType run_session_get_type(void) { return s_session_type; }

bool run_session_start_quick(void) {
  const AppSettings *settings = settings_get();
  if (!run_state_start()) {
    return false;
  }

  s_session_type = SESSION_QUICK;
  pace_engine_init(settings->target_pace_sec_per_km);
  pace_engine_reset();
  alert_engine_reset(&s_alert_state);
  hr_monitor_reset_avg();
  hr_monitor_start();
  comm_send_command(CMD_START);
  return true;
}

bool run_session_start_planned(uint8_t plan_index) {
  if (!plan_progress_start(&s_plan_progress, plan_index)) {
    return false;
  }
  if (!run_state_start()) {
    plan_progress_init(&s_plan_progress);
    return false;
  }

  s_session_type = SESSION_PLANNED;
  s_segment_start_elapsed = 0;
  s_segment_start_distance = 0;
  s_prev_segment_index = 0;
  alert_engine_reset(&s_alert_state);
  hr_monitor_reset_avg();
  hr_monitor_start();

  const PlanSegment *segment = plan_progress_current_segment(&s_plan_progress);
  apply_segment_targets(segment);
  pace_engine_reset();
  comm_send_command(CMD_START);
  return true;
}

void run_session_stop(void) {
  hr_monitor_stop();
  comm_send_command(CMD_STOP);
  s_session_type = SESSION_NONE;
  plan_progress_init(&s_plan_progress);
  alert_engine_reset(&s_alert_state);
}

void run_session_on_segment_change(void) {
  haptic_segment_change();
  const PlanSegment *segment = plan_progress_current_segment(&s_plan_progress);
  apply_segment_targets(segment);
  alert_engine_reset(&s_alert_state);
}

void run_session_on_pace_update(int32_t pace_sec_per_km) {
  AlertInput input;
  build_alert_input(&input, pace_sec_per_km);

  if (run_state_get() != RUN_ACTIVE) {
    return;
  }

  AlertType alert = alert_engine_check(&input);
  if (alert != ALERT_NONE) {
    return;
  }
}

void run_session_tick(void) {
  RunState state = run_state_get();
  if (state != RUN_ACTIVE) {
    return;
  }

  const RunStats *stats = run_state_get_stats();
  run_state_set_avg_hr(hr_monitor_get_avg_bpm());

  if (s_session_type == SESSION_PLANNED && s_plan_progress.active) {
    uint32_t seg_elapsed = stats->elapsed_seconds - s_segment_start_elapsed;
    uint32_t seg_distance = stats->distance_meters - s_segment_start_distance;

    uint8_t before = s_plan_progress.segment_index;
    plan_progress_tick(&s_plan_progress, seg_elapsed, seg_distance);

    if (s_plan_progress.segment_index != before) {
      s_segment_start_elapsed = stats->elapsed_seconds;
      s_segment_start_distance = stats->distance_meters;
      run_session_on_segment_change();
    }

    if (s_plan_progress.completed) {
      run_state_stop();
      run_session_stop();
      return;
    }
  }

  const PaceData *pace = pace_engine_get_data();
  AlertInput input;
  build_alert_input(&input, pace->current_pace_sec_per_km);

  AlertType alert = alert_engine_tick(&s_alert_state, &input);
  if (alert != ALERT_NONE) {
    haptic_fire_alert(alert);
  }
}

const PlanProgress *run_session_get_progress(void) { return &s_plan_progress; }

bool run_session_is_planned(void) {
  return s_session_type == SESSION_PLANNED;
}
