/**
 * run_window.h - Main run UI window
 */

#pragma once

#include <pebble.h>

/**
 * Create and push the run window.
 */
void run_window_push(void);

/**
 * Update the UI with new pace data.
 * Call this whenever pace data changes.
 */
void run_window_update(void);

/**
 * Show segment alert overlay.
 * @param segment_name Name of the segment
 * @param rival_name Name of the rival to beat
 */
void run_window_show_segment_alert(const char *segment_name,
                                   const char *rival_name);

/**
 * Hide segment alert overlay.
 */
void run_window_hide_segment_alert(void);
