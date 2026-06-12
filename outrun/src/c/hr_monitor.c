/**
 * hr_monitor.c - Heart rate monitoring via Pebble Health Service
 */

#include "hr_monitor.h"
#include <pebble.h>

static uint8_t s_current_bpm;
static uint32_t s_avg_bpm;
static uint32_t s_sample_count;
static bool s_available;
static bool s_debug_mode;
static uint8_t s_debug_bpm;

#if defined(PBL_HEALTH)
static void hr_handler(HealthEventType event, void *context) {
  (void)context;
  if (event != HealthEventHeartRateUpdate) {
    return;
  }

  HealthValue bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (bpm > 0 && bpm < 255) {
    s_current_bpm = (uint8_t)bpm;
    s_avg_bpm += bpm;
    s_sample_count++;
  }
}
#endif

void hr_monitor_init(void) {
  s_current_bpm = 0;
  s_avg_bpm = 0;
  s_sample_count = 0;
  s_debug_mode = false;
  s_debug_bpm = 0;
  s_available = false;

#if defined(PBL_HEALTH)
  if (health_service_events_subscribe(hr_handler, NULL)) {
    s_available = true;
  }
#endif
}

void hr_monitor_deinit(void) {
  hr_monitor_stop();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
}

void hr_monitor_start(void) {
#if defined(PBL_HEALTH)
  if (s_available) {
    health_service_set_heart_rate_sample_period(1);
  }
#endif
}

void hr_monitor_stop(void) {
#if defined(PBL_HEALTH)
  health_service_set_heart_rate_sample_period(0);
#endif
}

bool hr_monitor_is_available(void) {
  return s_available || s_debug_mode;
}

uint8_t hr_monitor_get_bpm(void) {
  if (s_debug_mode && s_debug_bpm > 0) {
    return s_debug_bpm;
  }
  return s_current_bpm;
}

uint32_t hr_monitor_get_avg_bpm(void) {
  if (s_sample_count == 0) {
    return 0;
  }
  return s_avg_bpm / s_sample_count;
}

void hr_monitor_set_debug_bpm(uint8_t bpm) {
  if (bpm > 0) {
    s_debug_mode = true;
    s_debug_bpm = bpm;
    s_current_bpm = bpm;
    s_avg_bpm += bpm;
    s_sample_count++;
  }
}

void hr_monitor_reset_avg(void) {
  s_avg_bpm = 0;
  s_sample_count = 0;
  s_current_bpm = 0;
}
