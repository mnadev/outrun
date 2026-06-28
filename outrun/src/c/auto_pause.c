/**
 * auto_pause.c - Debounced GPS auto-pause/resume decision (pure).
 */

#include "auto_pause.h"

void auto_pause_init(AutoPauseState *state) {
  if (!state) {
    return;
  }
  state->active = false;
  state->stopped_sec = 0;
}

bool auto_pause_tick(AutoPauseState *state, bool moving, uint16_t threshold_sec) {
  if (!state || state->active) {
    return false; // not tracking, or already paused -- the tick can't re-pause
  }
  if (moving) {
    state->stopped_sec = 0;
    return false;
  }
  state->stopped_sec++;
  if (state->stopped_sec >= threshold_sec) {
    state->active = true;
    return true;
  }
  return false;
}

bool auto_pause_on_moving(AutoPauseState *state, bool moving) {
  if (!state) {
    return false;
  }
  if (state->active && moving) {
    state->active = false;
    state->stopped_sec = 0;
    return true;
  }
  return false;
}

void auto_pause_reset(AutoPauseState *state) {
  if (!state) {
    return;
  }
  state->active = false;
  state->stopped_sec = 0;
}
