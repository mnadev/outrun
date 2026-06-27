/**
 * settings.h - User settings with persistent storage
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  UNITS_KM = 0,
  UNITS_MI = 1
} DistanceUnits;

// User-selectable accent color. ACCENT_THEME follows the active stalker theme;
// the rest force a specific hue. Stored as a plain integer so this header and
// settings.c stay free of GColor (resolved to a GColor in accent.c).
typedef enum {
  ACCENT_THEME = 0,
  ACCENT_RED,
  ACCENT_ORANGE,
  ACCENT_YELLOW,
  ACCENT_GREEN,
  ACCENT_CYAN,
  ACCENT_BLUE,
  ACCENT_MAGENTA,
  ACCENT_WHITE,
  ACCENT_COUNT
} AccentColor;

typedef struct {
  DistanceUnits units;
  int32_t target_pace_sec_per_km;
  uint8_t hr_zone_lo;
  uint8_t hr_zone_hi;
  bool pace_alerts_enabled;
  bool hr_alerts_enabled;
  uint8_t accent; // AccentColor
} AppSettings;

#define SETTINGS_PERSIST_KEY 1
#define DEFAULT_TARGET_PACE_SEC 300
#define DEFAULT_HR_ZONE_LO 120
#define DEFAULT_HR_ZONE_HI 150
// Valid heart-rate-zone bounds (bpm). The band is clamped into this range as a
// unit so stepping it up/down preserves its width at the edges.
#define HR_ZONE_MIN 60
#define HR_ZONE_MAX 220
#define METERS_PER_MILE 1609
// One target-pace button step, expressed in the user's display unit (seconds
// per km or per mile). Converted to sec/km via settings_pace_step_to_sec_per_km.
#define PACE_ADJUST_STEP_SEC 15

void settings_init(void);
const AppSettings *settings_get(void);
void settings_set_units(DistanceUnits units);
void settings_set_target_pace(int32_t pace_sec_per_km);
void settings_set_hr_zone(uint8_t lo, uint8_t hi);
void settings_set_pace_alerts(bool enabled);
void settings_set_hr_alerts(bool enabled);
void settings_set_accent(AccentColor accent);
void settings_save(void);

/** Human-readable name of an accent color (for the settings list). */
const char *settings_accent_name(AccentColor accent);

/** Convert sec/km to display sec per current unit (km or mile). */
int32_t settings_display_pace(int32_t pace_sec_per_km);

/** Convert display-unit pace back to sec/km. */
int32_t settings_pace_from_display(int32_t display_pace);

/**
 * Convert a target-pace step expressed in display units (sec per km or mile)
 * into a sec/km delta, so one button press changes the displayed pace by a
 * consistent amount regardless of units.
 */
int32_t settings_pace_step_to_sec_per_km(int32_t display_step_sec);

/** Format pace into buffer as m:ss per current unit. */
void settings_format_pace(char *buffer, size_t size, int32_t pace_sec_per_km);

/** Format distance in current units (km or mi, one decimal). */
void settings_format_distance(char *buffer, size_t size, uint32_t meters);
