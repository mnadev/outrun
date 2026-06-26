/**
 * test_hr_stats.c - Unit tests for the portable HR average accumulator.
 */

#include "../src/c/hr_stats.h"
#include "unity/unity.h"

static HrStats s_stats;

void setUp(void) { hr_stats_reset(&s_stats); }
void tearDown(void) {}

void test_empty_average_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT32(0, hr_stats_average(&s_stats));
}

void test_average_of_samples(void) {
  hr_stats_add(&s_stats, 100);
  hr_stats_add(&s_stats, 200);
  TEST_ASSERT_EQUAL_UINT32(150, hr_stats_average(&s_stats));
}

void test_zero_samples_are_ignored(void) {
  hr_stats_add(&s_stats, 150);
  hr_stats_add(&s_stats, 0); // "no reading" must not drag the average down
  TEST_ASSERT_EQUAL_UINT32(150, hr_stats_average(&s_stats));
}

void test_reset_clears_samples(void) {
  hr_stats_add(&s_stats, 180);
  hr_stats_reset(&s_stats);
  TEST_ASSERT_EQUAL_UINT32(0, hr_stats_average(&s_stats));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_average_is_zero);
  RUN_TEST(test_average_of_samples);
  RUN_TEST(test_zero_samples_are_ignored);
  RUN_TEST(test_reset_clears_samples);
  return UNITY_END();
}
