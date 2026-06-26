/**
 * feedback.h - Portable glue: semantic event -> pattern -> active watch.
 *
 * This is the "given an alert, do X" layer, with the platform-specific
 * X delegated to the injected WatchInterface. No SDK dependency.
 */

#pragma once

#include "haptic_patterns.h"

/** Look up the pattern for an event and play it on the active watch. */
void feedback_fire(HapticEvent event);

/** Cancel any in-progress vibration on the active watch. */
void feedback_cancel(void);
