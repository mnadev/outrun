/**
 * pace_engine.h - Pace calculation and "Distance from Killer" logic
 * 
 * Pure functions with no SDK dependencies for testability.
 * All times are in seconds, distances in meters.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Pace thresholds (in seconds tolerance from target)
#define PACE_TOLERANCE_SOFT 10   // Within ±10 sec = on target
#define PACE_TOLERANCE_DANGER 30 // Beyond 30 sec behind = danger zone

// Pace state relative to target
typedef enum {
    PACE_ON_TARGET,  // Within tolerance - you're safe (for now)
    PACE_AHEAD,      // Faster than target - outrunning the killer
    PACE_BEHIND,     // Slower than target - the killer is gaining
    PACE_DANGER      // Way behind - about to be caught!
} PaceState;

// Pace data structure
typedef struct {
    int32_t target_pace_sec_per_km;   // User's target pace
    int32_t current_pace_sec_per_km;  // Current calculated pace
    int32_t distance_from_killer;     // Conceptual distance (+ = safe, - = danger)
    PaceState state;                  // Current pace state
} PaceData;

/**
 * Initialize the pace engine with a target pace.
 * @param target_pace_sec_per_km Target pace in seconds per kilometer
 */
void pace_engine_init(int32_t target_pace_sec_per_km);

/**
 * Update the pace engine with current pace data.
 * @param current_pace_sec_per_km Current pace from GPS
 * @return Updated PaceState
 */
PaceState pace_engine_update(int32_t current_pace_sec_per_km);

/**
 * Get the current pace data.
 * @return Pointer to current PaceData (do not free)
 */
const PaceData* pace_engine_get_data(void);

/**
 * Adjust the target pace.
 * @param delta_sec Amount to adjust (positive = slower, negative = faster)
 */
void pace_engine_adjust_target(int32_t delta_sec);

/**
 * Reset the pace engine to initial state.
 */
void pace_engine_reset(void);

/**
 * Calculate pace state from delta (pure function for testing).
 * @param delta Difference between current and target pace (current - target)
 * @return The corresponding PaceState
 */
PaceState pace_calculate_state(int32_t delta);
