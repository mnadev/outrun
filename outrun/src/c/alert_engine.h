/**
 * alert_engine.h - Debounced pace/HR band alert logic (pure, testable)
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ALERT_DEBOUNCE_SEC 5
#define ALERT_REPEAT_SEC 20

typedef enum {
  ALERT_NONE = 0,
  ALERT_PACE_TOO_SLOW,
  ALERT_PACE_TOO_FAST,
  ALERT_HR_TOO_LOW,
  ALERT_HR_TOO_HIGH
} AlertType;

typedef struct {
  bool pace_alerts_enabled;
  bool hr_alerts_enabled;
  int32_t pace_lo_sec_per_km;
  int32_t pace_hi_sec_per_km;
  uint8_t hr_lo;
  uint8_t hr_hi;
  int32_t current_pace_sec_per_km;
  uint8_t current_hr;
  bool hr_available;
} AlertInput;

typedef struct {
  AlertType pending_alert;
  uint8_t out_of_band_seconds;
  uint8_t seconds_since_last_alert;
  AlertType last_fired;
} AlertState;

void alert_engine_init(AlertState *state);
AlertType alert_engine_tick(AlertState *state, const AlertInput *input);
void alert_engine_reset(AlertState *state);

/** Pure: determine instantaneous alert without debounce. */
AlertType alert_engine_check(const AlertInput *input);
