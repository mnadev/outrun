/**
 * test_auto_pause.c - Debounced auto-pause/resume state machine.
 */

#include "../src/c/auto_pause.h"
#include "unity/unity.h"

#define THRESHOLD 5

static AutoPauseState s;

void setUp(void) { auto_pause_init(&s); }
void tearDown(void) {}

void test_init_is_inactive(void) {
  TEST_ASSERT_FALSE(s.active);
  TEST_ASSERT_EQUAL_UINT16(0, s.stopped_sec);
}

void test_moving_never_pauses(void) {
  for (int i = 0; i < 100; i++) {
    TEST_ASSERT_FALSE(auto_pause_tick(&s, true, THRESHOLD));
  }
  TEST_ASSERT_FALSE(s.active);
}

void test_pauses_after_threshold_stopped_seconds(void) {
  // First THRESHOLD-1 stopped ticks do not pause yet.
  for (int i = 0; i < THRESHOLD - 1; i++) {
    TEST_ASSERT_FALSE(auto_pause_tick(&s, false, THRESHOLD));
  }
  // The THRESHOLD-th stopped tick triggers exactly once.
  TEST_ASSERT_TRUE(auto_pause_tick(&s, false, THRESHOLD));
  TEST_ASSERT_TRUE(s.active);
}

void test_does_not_repeat_pause_while_active(void) {
  for (int i = 0; i < THRESHOLD; i++) {
    auto_pause_tick(&s, false, THRESHOLD);
  }
  TEST_ASSERT_TRUE(s.active);
  // Further stopped ticks must not re-trigger.
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT_FALSE(auto_pause_tick(&s, false, THRESHOLD));
  }
}

void test_brief_movement_resets_the_stopped_counter(void) {
  auto_pause_tick(&s, false, THRESHOLD); // 1
  auto_pause_tick(&s, false, THRESHOLD); // 2
  auto_pause_tick(&s, true, THRESHOLD);  // reset to 0
  TEST_ASSERT_EQUAL_UINT16(0, s.stopped_sec);
  // Now it takes a full THRESHOLD again.
  for (int i = 0; i < THRESHOLD - 1; i++) {
    TEST_ASSERT_FALSE(auto_pause_tick(&s, false, THRESHOLD));
  }
  TEST_ASSERT_TRUE(auto_pause_tick(&s, false, THRESHOLD));
}

void test_resumes_on_movement_when_paused(void) {
  for (int i = 0; i < THRESHOLD; i++) {
    auto_pause_tick(&s, false, THRESHOLD);
  }
  TEST_ASSERT_TRUE(s.active);
  TEST_ASSERT_TRUE(auto_pause_on_moving(&s, true));
  TEST_ASSERT_FALSE(s.active);
}

void test_stays_paused_while_still(void) {
  for (int i = 0; i < THRESHOLD; i++) {
    auto_pause_tick(&s, false, THRESHOLD);
  }
  TEST_ASSERT_FALSE(auto_pause_on_moving(&s, false));
  TEST_ASSERT_TRUE(s.active);
}

void test_moving_when_active_runner_is_noop(void) {
  // Not auto-paused: a movement update should not report a resume.
  TEST_ASSERT_FALSE(auto_pause_on_moving(&s, true));
}

void test_reset_clears_state(void) {
  for (int i = 0; i < THRESHOLD; i++) {
    auto_pause_tick(&s, false, THRESHOLD);
  }
  TEST_ASSERT_TRUE(s.active);
  auto_pause_reset(&s);
  TEST_ASSERT_FALSE(s.active);
  TEST_ASSERT_EQUAL_UINT16(0, s.stopped_sec);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_init_is_inactive);
  RUN_TEST(test_moving_never_pauses);
  RUN_TEST(test_pauses_after_threshold_stopped_seconds);
  RUN_TEST(test_does_not_repeat_pause_while_active);
  RUN_TEST(test_brief_movement_resets_the_stopped_counter);
  RUN_TEST(test_resumes_on_movement_when_paused);
  RUN_TEST(test_stays_paused_while_still);
  RUN_TEST(test_moving_when_active_runner_is_noop);
  RUN_TEST(test_reset_clears_state);
  return UNITY_END();
}
