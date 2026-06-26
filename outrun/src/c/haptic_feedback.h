/**
 * haptic_feedback.h - Horror-themed vibration patterns.
 *
 * Public API used by the run loop. Implemented as thin wrappers that route
 * semantic events through the active WatchInterface, so this header carries
 * no SDK dependency (callers stay portable).
 */

#pragma once

#include <stdbool.h>

#include "alert_engine.h"

/**
 * Initialize the haptic feedback system (resets heartbeat state).
 */
void haptic_init(void);

/**
 * Deinitialize the haptic feedback system.
 */
void haptic_deinit(void);

/**
 * Soft pulse - used when on target pace.
 * Gentle reminder that you're safe... for now.
 */
void haptic_pulse_soft(void);

/**
 * Rapid heartbeat - used when falling behind.
 * The killer is gaining!
 */
void haptic_pulse_rapid(void);

/**
 * Danger pulse - used when critically behind.
 * You're about to be caught!
 */
void haptic_pulse_danger(void);

/**
 * Jump scare - used for segment alerts and rival notifications.
 * A long, aggressive vibration to get attention.
 */
void haptic_jump_scare(void);

/**
 * Start continuous heartbeat based on pace state.
 * Call this to begin rhythmic feedback during a run.
 * @param behind If true, uses rapid pattern; otherwise uses soft pattern
 */
void haptic_start_heartbeat(bool behind);

/**
 * Stop the continuous heartbeat.
 */
void haptic_stop_heartbeat(void);

/** Pacer alert patterns */
void haptic_pace_too_slow(void);
void haptic_pace_too_fast(void);
void haptic_hr_too_high(void);
void haptic_hr_too_low(void);
void haptic_segment_change(void);
void haptic_fire_alert(AlertType alert);
