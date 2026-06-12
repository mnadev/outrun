/**
 * test_plan.c - Unit tests for plan engine
 */

#include "../src/c/plan.h"
#include "unity/unity.h"

void setUp(void) { plan_store_init(); }

void tearDown(void) {}

void test_parse_segment_pace_target(void) {
  uint8_t data[] = {0x09, 0xB0, 0x04, 0x2C, 0x01, 0x4A, 0x01};
  PlanSegment segment;

  TEST_ASSERT_TRUE(plan_parse_segment(data, &segment));
  TEST_ASSERT_EQUAL(SEG_LABEL_RUN, segment.label);
  TEST_ASSERT_EQUAL(SEG_END_DURATION, segment.end_type);
  TEST_ASSERT_EQUAL(SEG_TARGET_PACE, segment.target_type);
  TEST_ASSERT_EQUAL(1200, segment.end_value);
  TEST_ASSERT_EQUAL(300, segment.target_lo);
  TEST_ASSERT_EQUAL(330, segment.target_hi);
}

void test_store_and_progress_duration_segment(void) {
  uint8_t packed[] = {0x09, 0x0A, 0x00, 0x2C, 0x01, 0x4A, 0x01};
  TEST_ASSERT_TRUE(plan_store_add(0, "Short", 1, packed, sizeof(packed)));

  PlanProgress progress;
  plan_progress_init(&progress);
  TEST_ASSERT_TRUE(plan_progress_start(&progress, 0));

  for (uint32_t i = 1; i < 10; i++) {
    plan_progress_tick(&progress, i, 0);
    TEST_ASSERT_TRUE(progress.active);
  }

  plan_progress_tick(&progress, 10, 0);
  TEST_ASSERT_TRUE(progress.completed);
}

void test_defaults_loaded(void) {
  plan_store_load_defaults();
  TEST_ASSERT_TRUE(plan_store_count() >= 2);
  TEST_ASSERT_NOT_NULL(plan_store_get(0));
}

void test_progress_distance_segment(void) {
  uint8_t packed[] = {0x0D, 0x90, 0x01, 0x0E, 0x01, 0x1D, 0x01};
  TEST_ASSERT_TRUE(plan_store_add(0, "400m", 1, packed, sizeof(packed)));

  PlanProgress progress;
  plan_progress_init(&progress);
  TEST_ASSERT_TRUE(plan_progress_start(&progress, 0));

  plan_progress_tick(&progress, 10, 399);
  TEST_ASSERT_TRUE(progress.active);
  TEST_ASSERT_EQUAL(1, plan_progress_remaining(&progress));

  plan_progress_tick(&progress, 12, 400);
  TEST_ASSERT_TRUE(progress.completed);
}

void test_progress_overshoot_advances_once(void) {
  uint8_t packed[] = {0x09, 0x05, 0x00, 0x2C, 0x01, 0x4A, 0x01};
  TEST_ASSERT_TRUE(plan_store_add(0, "FiveSec", 1, packed, sizeof(packed)));

  PlanProgress progress;
  plan_progress_init(&progress);
  TEST_ASSERT_TRUE(plan_progress_start(&progress, 0));

  plan_progress_tick(&progress, 10, 0);
  TEST_ASSERT_TRUE(progress.completed);
  TEST_ASSERT_FALSE(progress.active);
}

void test_store_rejects_short_data(void) {
  uint8_t packed[] = {0x09, 0xB0, 0x04};
  TEST_ASSERT_FALSE(plan_store_add(0, "Bad", 1, packed, sizeof(packed)));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_segment_pace_target);
  RUN_TEST(test_store_and_progress_duration_segment);
  RUN_TEST(test_defaults_loaded);
  RUN_TEST(test_progress_distance_segment);
  RUN_TEST(test_progress_overshoot_advances_once);
  RUN_TEST(test_store_rejects_short_data);
  return UNITY_END();
}
