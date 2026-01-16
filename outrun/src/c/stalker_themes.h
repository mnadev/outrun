/**
 * stalker_themes.h - Custom stalker themes (Premium Feature)
 *
 * Different horror themes with unique vibration patterns
 * and display styles for premium users.
 */

#pragma once

#include <pebble.h>

// Theme IDs
typedef enum {
  THEME_CLASSIC = 0, // Default slasher movie theme
  THEME_PARANORMAL,  // Ghost/supernatural theme
  THEME_ZOMBIE,      // Zombie apocalypse theme
  THEME_ALIEN,       // Sci-fi alien pursuit theme
  THEME_WEREWOLF,    // Full moon chase theme
  THEME_COUNT
} StalkerTheme;

// Theme configuration (colors handled by getter functions)
typedef struct {
  StalkerTheme id;
  const char *name;
  const char *stalker_name;   // What's chasing you
  const char *escape_message; // Victory message
  const char *caught_message; // Defeat message
  const char *behind_message; // "IT'S GAINING" equivalent
  const char *ahead_message;  // "OUTRUNNING" equivalent
} ThemeConfig;

/**
 * Initialize themes module
 */
void themes_init(void);

/**
 * Get current theme
 */
StalkerTheme themes_get_current(void);

/**
 * Set current theme (requires premium)
 */
bool themes_set_current(StalkerTheme theme);

/**
 * Get theme configuration
 */
const ThemeConfig *themes_get_config(StalkerTheme theme);

/**
 * Get current theme's configuration
 */
const ThemeConfig *themes_get_current_config(void);

/**
 * Get primary color for a theme
 */
GColor themes_get_primary_color(StalkerTheme theme);

/**
 * Get danger color for a theme
 */
GColor themes_get_danger_color(StalkerTheme theme);

/**
 * Apply theme-specific haptic pattern for "pulse"
 */
void themes_haptic_pulse(void);

/**
 * Apply theme-specific haptic pattern for "danger"
 */
void themes_haptic_danger(void);

/**
 * Apply theme-specific haptic pattern for "jump scare"
 */
void themes_haptic_scare(void);
