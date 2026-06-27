/**
 * accent.h - Resolve the user's accent setting to a drawable GColor.
 *
 * Kept separate from settings.c so settings stays GColor-free (host-testable).
 */

#pragma once

#include <pebble.h>

/**
 * The accent color to use for highlights (menus, killer bar, pace picker).
 * Resolves ACCENT_THEME to the active stalker theme's primary color, and
 * always returns white on black-and-white watches.
 */
GColor accent_get_color(void);
