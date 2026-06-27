/**
 * pace_engine.c - Pace calculation and "Distance from Killer" logic
 */

#include "pace_engine.h"

// Static pace data
static PaceData s_pace_data;

// Pure function - calculates state from delta
PaceState pace_calculate_state(int32_t delta) {
  if (delta < -PACE_TOLERANCE_SOFT) {
    // Running faster than target (negative delta = ahead)
    return PACE_AHEAD;
  } else if (delta <= PACE_TOLERANCE_SOFT) {
    // Within tolerance
    return PACE_ON_TARGET;
  } else if (delta <= PACE_TOLERANCE_DANGER) {
    // Behind but not critical
    return PACE_BEHIND;
  } else {
    // Way behind - danger zone!
    return PACE_DANGER;
  }
}

void pace_engine_init(int32_t target_pace_sec_per_km) {
  s_pace_data.target_pace_sec_per_km = target_pace_sec_per_km;
  s_pace_data.current_pace_sec_per_km = target_pace_sec_per_km;
  s_pace_data.distance_from_killer = 100; // Start with a buffer
  s_pace_data.state = PACE_ON_TARGET;
}

void pace_engine_set_target(int32_t target_pace_sec_per_km) {
  if (target_pace_sec_per_km < 180) {
    target_pace_sec_per_km = 180;
  }
  if (target_pace_sec_per_km > 900) {
    target_pace_sec_per_km = 900;
  }
  s_pace_data.target_pace_sec_per_km = target_pace_sec_per_km;
}

PaceState pace_engine_update(int32_t current_pace_sec_per_km) {
  s_pace_data.current_pace_sec_per_km = current_pace_sec_per_km;

  // Calculate delta: positive means behind, negative means ahead
  int32_t delta = current_pace_sec_per_km - s_pace_data.target_pace_sec_per_km;

  // Update state
  s_pace_data.state = pace_calculate_state(delta);

  // Update "distance from killer" - decreases when behind, increases when
  // ahead. A deadband within PACE_TOLERANCE_SOFT holds the lead steady while
  // you're on target, so the bar doesn't slowly drift when you're pacing well.
  if (delta > PACE_TOLERANCE_SOFT) {
    // Behind pace - killer is gaining
    s_pace_data.distance_from_killer -= (delta / 5);
    if (s_pace_data.distance_from_killer < 0) {
      s_pace_data.distance_from_killer = 0;
    }
  } else if (delta < -PACE_TOLERANCE_SOFT) {
    // Ahead of pace - extending lead
    s_pace_data.distance_from_killer -=
        (delta / 5); // delta is negative, so this adds
    if (s_pace_data.distance_from_killer > 200) {
      s_pace_data.distance_from_killer = 200; // Cap the lead
    }
  }
  // else: within tolerance (on target) -> hold the lead steady.

  return s_pace_data.state;
}

const PaceData *pace_engine_get_data(void) { return &s_pace_data; }

void pace_engine_adjust_target(int32_t delta_sec) {
  s_pace_data.target_pace_sec_per_km += delta_sec;

  // Clamp to reasonable values (3:00 to 15:00 per km)
  if (s_pace_data.target_pace_sec_per_km < 180) {
    s_pace_data.target_pace_sec_per_km = 180;
  }
  if (s_pace_data.target_pace_sec_per_km > 900) {
    s_pace_data.target_pace_sec_per_km = 900;
  }
}

void pace_engine_reset(void) {
  s_pace_data.distance_from_killer = 100;
  s_pace_data.state = PACE_ON_TARGET;
}
