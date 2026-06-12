/**
 * alert_engine.c - Debounced pace/HR band alert logic
 */

#include "alert_engine.h"

void alert_engine_init(AlertState *state) {
  state->pending_alert = ALERT_NONE;
  state->out_of_band_seconds = 0;
  state->seconds_since_last_alert = 0;
  state->last_fired = ALERT_NONE;
}

AlertType alert_engine_check(const AlertInput *input) {
  if (input->hr_available && input->hr_alerts_enabled && input->current_hr > 0) {
    if (input->current_hr > input->hr_hi) {
      return ALERT_HR_TOO_HIGH;
    }
    if (input->current_hr < input->hr_lo) {
      return ALERT_HR_TOO_LOW;
    }
  }

  if (input->pace_alerts_enabled && input->current_pace_sec_per_km > 0) {
    if (input->current_pace_sec_per_km > input->pace_hi_sec_per_km) {
      return ALERT_PACE_TOO_SLOW;
    }
    if (input->current_pace_sec_per_km < input->pace_lo_sec_per_km) {
      return ALERT_PACE_TOO_FAST;
    }
  }

  return ALERT_NONE;
}

AlertType alert_engine_tick(AlertState *state, const AlertInput *input) {
  AlertType instant = alert_engine_check(input);

  if (instant == ALERT_NONE) {
    state->pending_alert = ALERT_NONE;
    state->out_of_band_seconds = 0;
    state->seconds_since_last_alert = 0;
    state->last_fired = ALERT_NONE;
    return ALERT_NONE;
  }

  if (instant != state->pending_alert) {
    state->pending_alert = instant;
    state->out_of_band_seconds = 1;
    state->seconds_since_last_alert = 0;
    return ALERT_NONE;
  }

  state->out_of_band_seconds++;

  if (state->out_of_band_seconds < ALERT_DEBOUNCE_SEC) {
    return ALERT_NONE;
  }

  state->seconds_since_last_alert++;

  if (state->last_fired == instant &&
      state->seconds_since_last_alert < ALERT_REPEAT_SEC) {
    return ALERT_NONE;
  }

  state->last_fired = instant;
  state->seconds_since_last_alert = 0;
  return instant;
}

void alert_engine_reset(AlertState *state) { alert_engine_init(state); }
