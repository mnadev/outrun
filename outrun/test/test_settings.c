/**
 * test_settings.c - Settings persistence (versioned blob) and unit-aware
 * target-pace stepping.
 */

#include "../src/c/settings.h"
#include "support/persist_stub.h"
#include "unity/unity.h"

void setUp(void) { persist_stub_reset(); }
void tearDown(void) {}

// --- Persistence -----------------------------------------------------------

void test_defaults_when_storage_empty(void) {
  settings_init();
  const AppSettings *s = settings_get();
  TEST_ASSERT_EQUAL_INT(UNITS_KM, s->units);
  TEST_ASSERT_EQUAL_INT32(DEFAULT_TARGET_PACE_SEC, s->target_pace_sec_per_km);
  TEST_ASSERT_EQUAL_UINT8(DEFAULT_HR_ZONE_LO, s->hr_zone_lo);
  TEST_ASSERT_EQUAL_UINT8(DEFAULT_HR_ZONE_HI, s->hr_zone_hi);
  TEST_ASSERT_TRUE(s->pace_alerts_enabled);
  TEST_ASSERT_TRUE(s->hr_alerts_enabled);
}

void test_persist_round_trip(void) {
  settings_init();
  settings_set_units(UNITS_MI);
  settings_set_target_pace(360);
  settings_set_hr_zone(130, 160);
  settings_set_pace_alerts(false);
  settings_set_accent(ACCENT_CYAN);

  // Re-init reads back the versioned blob (simulates relaunch).
  settings_init();
  const AppSettings *s = settings_get();
  TEST_ASSERT_EQUAL_INT(UNITS_MI, s->units);
  TEST_ASSERT_EQUAL_INT32(360, s->target_pace_sec_per_km);
  TEST_ASSERT_EQUAL_UINT8(130, s->hr_zone_lo);
  TEST_ASSERT_EQUAL_UINT8(160, s->hr_zone_hi);
  TEST_ASSERT_FALSE(s->pace_alerts_enabled);
  TEST_ASSERT_EQUAL_INT(ACCENT_CYAN, s->accent);
}

void test_accent_defaults_to_theme(void) {
  settings_init();
  TEST_ASSERT_EQUAL_INT(ACCENT_THEME, settings_get()->accent);
}

void test_accent_rejects_out_of_range(void) {
  settings_init();
  settings_set_accent(ACCENT_GREEN);
  settings_set_accent((AccentColor)999); // ignored
  TEST_ASSERT_EQUAL_INT(ACCENT_GREEN, settings_get()->accent);
}

void test_legacy_unversioned_blob_is_rejected(void) {
  // Simulate an older build that persisted a bare AppSettings (no magic).
  AppSettings legacy;
  legacy.units = UNITS_MI;
  legacy.target_pace_sec_per_km = 815;
  legacy.hr_zone_lo = 99;
  legacy.hr_zone_hi = 188;
  legacy.pace_alerts_enabled = false;
  legacy.hr_alerts_enabled = false;
  persist_stub_write_raw(SETTINGS_PERSIST_KEY, &legacy, sizeof(legacy));

  settings_init();
  const AppSettings *s = settings_get();
  // Must ignore the unversioned data and use safe defaults instead.
  TEST_ASSERT_EQUAL_INT(UNITS_KM, s->units);
  TEST_ASSERT_EQUAL_INT32(DEFAULT_TARGET_PACE_SEC, s->target_pace_sec_per_km);
}

void test_garbage_blob_is_rejected(void) {
  uint8_t junk[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  persist_stub_write_raw(SETTINGS_PERSIST_KEY, junk, sizeof(junk));

  settings_init();
  const AppSettings *s = settings_get();
  TEST_ASSERT_EQUAL_INT(UNITS_KM, s->units);
  TEST_ASSERT_EQUAL_INT32(DEFAULT_TARGET_PACE_SEC, s->target_pace_sec_per_km);
}

// --- Unit-aware pace stepping ---------------------------------------------

void test_pace_step_km_is_full_seconds(void) {
  settings_init();
  settings_set_units(UNITS_KM);
  TEST_ASSERT_EQUAL_INT32(PACE_ADJUST_STEP_SEC,
                          settings_pace_step_to_sec_per_km(PACE_ADJUST_STEP_SEC));
  TEST_ASSERT_EQUAL_INT32(-PACE_ADJUST_STEP_SEC,
                          settings_pace_step_to_sec_per_km(-PACE_ADJUST_STEP_SEC));
}

void test_pace_step_mi_is_smaller_and_symmetric(void) {
  settings_init();
  settings_set_units(UNITS_MI);
  int32_t up = settings_pace_step_to_sec_per_km(PACE_ADJUST_STEP_SEC);
  int32_t down = settings_pace_step_to_sec_per_km(-PACE_ADJUST_STEP_SEC);
  // 15s/mile is a smaller sec/km change, and up/down are exact inverses.
  TEST_ASSERT_TRUE(up > 0 && up < PACE_ADJUST_STEP_SEC);
  TEST_ASSERT_EQUAL_INT32(up, -down);
}

void test_pace_step_mi_moves_display_by_about_one_step(void) {
  settings_init();
  settings_set_units(UNITS_MI);
  int32_t before = settings_display_pace(300);
  int32_t after =
      settings_display_pace(300 + settings_pace_step_to_sec_per_km(PACE_ADJUST_STEP_SEC));
  // The displayed min/mile value should move by ~15s, not ~24s as before.
  TEST_ASSERT_INT_WITHIN(2, PACE_ADJUST_STEP_SEC, after - before);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_when_storage_empty);
  RUN_TEST(test_persist_round_trip);
  RUN_TEST(test_accent_defaults_to_theme);
  RUN_TEST(test_accent_rejects_out_of_range);
  RUN_TEST(test_legacy_unversioned_blob_is_rejected);
  RUN_TEST(test_garbage_blob_is_rejected);
  RUN_TEST(test_pace_step_km_is_full_seconds);
  RUN_TEST(test_pace_step_mi_is_smaller_and_symmetric);
  RUN_TEST(test_pace_step_mi_moves_display_by_about_one_step);
  return UNITY_END();
}
