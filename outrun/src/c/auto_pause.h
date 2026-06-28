/**
 * auto_pause.h - Debounced GPS auto-pause/resume decision (pure, no SDK).
 *
 * The phone reports whether the runner is moving (derived from the Kalman
 * speed estimate). This module decides WHEN to auto-pause (after the runner has
 * been stopped for a number of consecutive seconds) and WHEN to auto-resume (as
 * soon as movement returns). The side effects (pausing the run, stopping HR,
 * UI) live in run_session/run_window; this is just the testable state machine.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool active;          // currently auto-paused
  uint16_t stopped_sec; // consecutive stopped seconds while running
} AutoPauseState;

void auto_pause_init(AutoPauseState *state);

/**
 * Call once per active second with the latest movement state.
 * @return true the moment an auto-pause should trigger (stopped for
 *         threshold_sec consecutive seconds). Returns false once already paused.
 */
bool auto_pause_tick(AutoPauseState *state, bool moving, uint16_t threshold_sec);

/**
 * Call when a movement update arrives (any run state).
 * @return true the moment an auto-resume should trigger (was auto-paused and is
 *         moving again).
 */
bool auto_pause_on_moving(AutoPauseState *state, bool moving);

/** Forget any auto-pause (manual pause/resume/stop, or a new run). */
void auto_pause_reset(AutoPauseState *state);
