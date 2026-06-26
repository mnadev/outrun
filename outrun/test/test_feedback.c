/**
 * test_feedback.c - Routing tests: event -> pattern -> watch interface.
 *
 * Exercises the glue with a mock watch, proving the alert-to-haptic path
 * works with no SDK present.
 */

#include "../src/c/feedback.h"
#include "../src/c/haptic_patterns.h"
#include "../src/c/watch_interface.h"
#include "support/mock_watch.h"
#include "unity/unity.h"

void setUp(void) {
  mock_watch_install();
  mock_watch_reset();
}

void tearDown(void) {}

void test_fire_routes_correct_pattern(void) {
  feedback_fire(HAPTIC_PACE_TOO_SLOW);
  TEST_ASSERT_EQUAL_UINT32(1, mock_watch_play_count());
  TEST_ASSERT_EQUAL_PTR(haptic_pattern_for(HAPTIC_PACE_TOO_SLOW),
                        mock_watch_last_pattern());
}

void test_alert_path_routes_through_mapping(void) {
  feedback_fire(haptic_event_for_alert(ALERT_HR_TOO_HIGH));
  TEST_ASSERT_EQUAL_PTR(haptic_pattern_for(HAPTIC_HR_TOO_HIGH),
                        mock_watch_last_pattern());
}

void test_fire_none_plays_nothing(void) {
  feedback_fire(HAPTIC_NONE);
  TEST_ASSERT_EQUAL_UINT32(0, mock_watch_play_count());
}

void test_cancel_reaches_watch(void) {
  feedback_cancel();
  TEST_ASSERT_EQUAL_UINT32(1, mock_watch_cancel_count());
}

void test_no_watch_is_safe(void) {
  watch_set(NULL);
  feedback_fire(HAPTIC_DANGER);
  feedback_cancel();
  TEST_ASSERT_EQUAL_UINT32(0, mock_watch_play_count());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_fire_routes_correct_pattern);
  RUN_TEST(test_alert_path_routes_through_mapping);
  RUN_TEST(test_fire_none_plays_nothing);
  RUN_TEST(test_cancel_reaches_watch);
  RUN_TEST(test_no_watch_is_safe);
  return UNITY_END();
}
