/**
 * run_state.h - Run state machine
 *
 * Manages the run lifecycle: idle -> active -> paused -> complete
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Run states
typedef enum {
  RUN_IDLE,    // Not running, ready to start
  RUN_ACTIVE,  // Actively running
  RUN_PAUSED,  // Temporarily paused
  RUN_COMPLETE // Run finished
} RunState;

// Run statistics
typedef struct {
  uint32_t elapsed_seconds;     // Total time running
  uint32_t distance_meters;     // Total distance (from phone GPS)
  uint32_t avg_pace_sec_per_km; // Average pace
  uint32_t avg_hr_bpm;          // Average heart rate
  bool escaped;                 // Did we outrun the killer?
} RunStats;

/**
 * Initialize the run state machine.
 */
void run_state_init(void);

/**
 * Deinitialize the run state machine.
 */
void run_state_deinit(void);

/**
 * Start a new run.
 * @return true if state changed, false if already running
 */
bool run_state_start(void);

/**
 * Pause the current run.
 * @return true if state changed, false if not running
 */
bool run_state_pause(void);

/**
 * Resume a paused run.
 * @return true if state changed, false if not paused
 */
bool run_state_resume(void);

/**
 * Stop and complete the run.
 * @return true if state changed, false if not running
 */
bool run_state_stop(void);

/**
 * Reset to idle state.
 */
void run_state_reset(void);

/**
 * Get the current run state.
 */
RunState run_state_get(void);

/**
 * Get the current run statistics.
 */
const RunStats *run_state_get_stats(void);

/**
 * Update elapsed time (call every second during RUN_ACTIVE).
 */
void run_state_tick(void);

/**
 * Update distance (called when GPS data arrives).
 * @param meters Distance traveled in meters
 */
void run_state_add_distance(uint32_t meters);

/**
 * Set total distance from phone GPS snapshot.
 */
void run_state_set_distance(uint32_t meters);

/**
 * Update average heart rate.
 */
void run_state_set_avg_hr(uint32_t bpm);
