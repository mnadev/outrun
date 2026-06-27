/**
 * test_haptic_patterns.c - Unit tests for the portable pattern table.
 */

#include "../src/c/haptic_patterns.h"
#include "unity/unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_pace_patterns_have_expected_shape(void) {
  const HapticPattern *slow = haptic_pattern_for(HAPTIC_PACE_TOO_SLOW);
  TEST_ASSERT_NOT_NULL(slow);
  TEST_ASSERT_EQUAL_UINT32(5, slow->count);
  TEST_ASSERT_EQUAL_UINT32(80, slow->durations[0]);

  const HapticPattern *fast = haptic_pattern_for(HAPTIC_PACE_TOO_FAST);
  TEST_ASSERT_NOT_NULL(fast);
  TEST_ASSERT_EQUAL_UINT32(1, fast->count);
  TEST_ASSERT_EQUAL_UINT32(250, fast->durations[0]);
}

void test_none_returns_null(void) {
  TEST_ASSERT_NULL(haptic_pattern_for(HAPTIC_NONE));
}

void test_out_of_range_returns_null(void) {
  TEST_ASSERT_NULL(haptic_pattern_for(HAPTIC_EVENT_COUNT));
  TEST_ASSERT_NULL(haptic_pattern_for((HapticEvent)999));
}

void test_every_real_event_has_a_pattern(void) {
  for (int e = HAPTIC_NONE + 1; e < HAPTIC_EVENT_COUNT; e++) {
    const HapticPattern *pattern = haptic_pattern_for((HapticEvent)e);
    TEST_ASSERT_NOT_NULL(pattern);
    TEST_ASSERT_TRUE(pattern->count > 0);
  }
}

void test_alert_to_event_mapping(void) {
  TEST_ASSERT_EQUAL(HAPTIC_PACE_TOO_SLOW,
                    haptic_event_for_alert(ALERT_PACE_TOO_SLOW));
  TEST_ASSERT_EQUAL(HAPTIC_PACE_TOO_FAST,
                    haptic_event_for_alert(ALERT_PACE_TOO_FAST));
  TEST_ASSERT_EQUAL(HAPTIC_HR_TOO_HIGH,
                    haptic_event_for_alert(ALERT_HR_TOO_HIGH));
  TEST_ASSERT_EQUAL(HAPTIC_HR_TOO_LOW,
                    haptic_event_for_alert(ALERT_HR_TOO_LOW));
  TEST_ASSERT_EQUAL(HAPTIC_NONE, haptic_event_for_alert(ALERT_NONE));
}

// The heartbeat loop re-enqueues the rapid pattern every
// HAPTIC_HEARTBEAT_RAPID_MS. If the pattern were longer than the interval,
// vibes would queue patterns back-to-back into continuous vibration (battery
// drain / unusable). Guard that regression.
void test_rapid_heartbeat_pattern_fits_within_interval(void) {
  const HapticPattern *rapid = haptic_pattern_for(HAPTIC_HEARTBEAT_RAPID);
  TEST_ASSERT_NOT_NULL(rapid);
  uint32_t duration_ms = 0;
  for (uint32_t i = 0; i < rapid->count; i++) {
    duration_ms += rapid->durations[i];
  }
  TEST_ASSERT_TRUE(duration_ms < HAPTIC_HEARTBEAT_RAPID_MS);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_pace_patterns_have_expected_shape);
  RUN_TEST(test_none_returns_null);
  RUN_TEST(test_out_of_range_returns_null);
  RUN_TEST(test_every_real_event_has_a_pattern);
  RUN_TEST(test_alert_to_event_mapping);
  RUN_TEST(test_rapid_heartbeat_pattern_fits_within_interval);
  return UNITY_END();
}
