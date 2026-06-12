/**
 * settings.c - User settings with persistent storage
 */

#include "settings.h"
#include <pebble.h>
#include <stdio.h>

static AppSettings s_settings;

void settings_init(void) {
  if (persist_exists(SETTINGS_PERSIST_KEY)) {
    persist_read_data(SETTINGS_PERSIST_KEY, &s_settings, sizeof(AppSettings));
  } else {
    s_settings.units = UNITS_KM;
    s_settings.target_pace_sec_per_km = DEFAULT_TARGET_PACE_SEC;
    s_settings.hr_zone_lo = DEFAULT_HR_ZONE_LO;
    s_settings.hr_zone_hi = DEFAULT_HR_ZONE_HI;
    s_settings.pace_alerts_enabled = true;
    s_settings.hr_alerts_enabled = true;
    settings_save();
  }

  if (s_settings.hr_zone_lo >= s_settings.hr_zone_hi) {
    s_settings.hr_zone_lo = DEFAULT_HR_ZONE_LO;
    s_settings.hr_zone_hi = DEFAULT_HR_ZONE_HI;
  }
}

const AppSettings *settings_get(void) { return &s_settings; }

void settings_set_units(DistanceUnits units) {
  s_settings.units = units;
  settings_save();
}

void settings_set_target_pace(int32_t pace_sec_per_km) {
  if (pace_sec_per_km < 180) {
    pace_sec_per_km = 180;
  }
  if (pace_sec_per_km > 900) {
    pace_sec_per_km = 900;
  }
  s_settings.target_pace_sec_per_km = pace_sec_per_km;
  settings_save();
}

void settings_adjust_target_pace(int32_t delta_sec) {
  settings_set_target_pace(s_settings.target_pace_sec_per_km + delta_sec);
}

void settings_set_hr_zone(uint8_t lo, uint8_t hi) {
  if (lo < 60) {
    lo = 60;
  }
  if (hi > 220) {
    hi = 220;
  }
  if (lo >= hi) {
    return;
  }
  s_settings.hr_zone_lo = lo;
  s_settings.hr_zone_hi = hi;
  settings_save();
}

void settings_set_pace_alerts(bool enabled) {
  s_settings.pace_alerts_enabled = enabled;
  settings_save();
}

void settings_set_hr_alerts(bool enabled) {
  s_settings.hr_alerts_enabled = enabled;
  settings_save();
}

void settings_save(void) {
  persist_write_data(SETTINGS_PERSIST_KEY, &s_settings, sizeof(AppSettings));
}

int32_t settings_display_pace(int32_t pace_sec_per_km) {
  if (s_settings.units == UNITS_MI) {
    return (pace_sec_per_km * METERS_PER_MILE) / 1000;
  }
  return pace_sec_per_km;
}

int32_t settings_pace_from_display(int32_t display_pace) {
  if (s_settings.units == UNITS_MI) {
    return (display_pace * 1000) / METERS_PER_MILE;
  }
  return display_pace;
}

void settings_format_pace(char *buffer, size_t size, int32_t pace_sec_per_km) {
  int32_t display = settings_display_pace(pace_sec_per_km);
  int minutes = display / 60;
  int seconds = display % 60;
  snprintf(buffer, size, "%d:%02d", minutes, seconds);
}

void settings_format_distance(char *buffer, size_t size, uint32_t meters) {
  if (s_settings.units == UNITS_MI) {
    uint32_t whole = meters / METERS_PER_MILE;
    uint32_t tenths = ((meters % METERS_PER_MILE) * 10) / METERS_PER_MILE;
    snprintf(buffer, size, "%lu.%lu mi", (unsigned long)whole,
             (unsigned long)tenths);
  } else {
    uint32_t whole = meters / 1000;
    uint32_t tenths = (meters % 1000) / 100;
    snprintf(buffer, size, "%lu.%lu km", (unsigned long)whole,
             (unsigned long)tenths);
  }
}
