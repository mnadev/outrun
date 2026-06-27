/**
 * run_window.h - Main run UI window
 */

#pragma once

#include <pebble.h>

void run_window_push_quick(void);
void run_window_push_planned(uint8_t plan_index);
void run_window_update(void);
void run_window_show_summary(void);

/** Mark that a live pace update just arrived from the phone (clears the
 *  "Acquiring GPS" / "GPS lost" state and refreshes the screen). */
void run_window_note_live_pace(void);
void run_window_show_segment_alert(const char *segment_name, const char *rival_name);
void run_window_hide_segment_alert(void);
