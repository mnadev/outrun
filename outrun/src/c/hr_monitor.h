/**
 * hr_monitor.h - Heart rate monitoring via Pebble Health Service
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

void hr_monitor_init(void);
void hr_monitor_deinit(void);
void hr_monitor_start(void);
void hr_monitor_stop(void);
bool hr_monitor_is_available(void);
uint8_t hr_monitor_get_bpm(void);

/**
 * Inject a phone-supplied heart rate (companion HEART_RATE message). Used in
 * place of the on-watch sensor until cleared on the next hr_monitor_stop().
 */
void hr_monitor_set_phone_bpm(uint8_t bpm);
