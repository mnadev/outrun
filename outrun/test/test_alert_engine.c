/**
 * test_alert_engine.c - Unit tests for alert engine
 */

#include "../src/c/alert_engine.h"
#include "unity/unity.h"

static AlertState s_state;

void setUp(void) { alert_engine_init(&s_state); }

void tearDown(void) {}

static AlertInput make_input(int32_t pace, uint8_t hr) {
  AlertInput input;
  input.pace_alerts_enabled = true;
  input.hr_alerts_enabled = true;
  input.pace_lo_sec_per_km = 290;
  input.pace_hi_sec_per_km = 310;
  input.hr_lo = 120;
  input.hr_hi = 150;
  input.current_pace_sec_per_km = pace;
  input.current_hr = hr;
  input.hr_available = true;
  return input;
}

void test_check_pace_too_slow(void) {
  AlertInput input = make_input(320, 130);
  TEST_ASSERT_EQUAL(ALERT_PACE_TOO_SLOW, alert_engine_check(&input));
}

void test_check_pace_too_fast(void) {
  AlertInput input = make_input(280, 130);
  TEST_ASSERT_EQUAL(ALERT_PACE_TOO_FAST, alert_engine_check(&input));
}

void test_check_hr_takes_priority(void) {
  AlertInput input = make_input(320, 160);
  TEST_ASSERT_EQUAL(ALERT_HR_TOO_HIGH, alert_engine_check(&input));
}

void test_debounce_requires_five_seconds(void) {
  AlertInput input = make_input(320, 130);

  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL(ALERT_NONE, alert_engine_tick(&s_state, &input));
  }

  TEST_ASSERT_EQUAL(ALERT_PACE_TOO_SLOW, alert_engine_tick(&s_state, &input));
}

void test_repeat_waits_twenty_seconds(void) {
  AlertInput input = make_input(320, 130);

  for (int i = 0; i < 5; i++) {
    alert_engine_tick(&s_state, &input);
  }
  TEST_ASSERT_EQUAL(ALERT_PACE_TOO_SLOW, s_state.last_fired);

  for (int i = 0; i < 19; i++) {
    TEST_ASSERT_EQUAL(ALERT_NONE, alert_engine_tick(&s_state, &input));
  }

  TEST_ASSERT_EQUAL(ALERT_PACE_TOO_SLOW, alert_engine_tick(&s_state, &input));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_check_pace_too_slow);
  RUN_TEST(test_check_pace_too_fast);
  RUN_TEST(test_check_hr_takes_priority);
  RUN_TEST(test_debounce_requires_five_seconds);
  RUN_TEST(test_repeat_waits_twenty_seconds);
  return UNITY_END();
}
