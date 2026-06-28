/**
 * run_session.h - Coordinates quick runs and planned runs
 */

#pragma once

#include "alert_engine.h"
#include "plan.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SESSION_NONE = 0,
  SESSION_QUICK,
  SESSION_PLANNED
} SessionType;

void run_session_init(void);
void run_session_prepare_planned(uint8_t plan_index);
uint8_t run_session_pending_plan_index(void);
SessionType run_session_get_type(void);
bool run_session_start_quick(void);
bool run_session_start_planned(uint8_t plan_index);
void run_session_stop(void);
void run_session_tick(void);
void run_session_on_segment_change(void);
const PlanProgress *run_session_get_progress(void);
bool run_session_is_planned(void);
bool run_session_is_active(void);

/** Phone-reported movement state, used for GPS auto-pause/resume. */
void run_session_set_moving(bool moving);
/** True while the run is paused by auto-pause (vs. a manual pause). */
bool run_session_is_auto_paused(void);
/** Clear auto-pause bookkeeping on a manual pause/resume/stop. */
void run_session_reset_auto_pause(void);
