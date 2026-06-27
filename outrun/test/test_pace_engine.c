/**
 * test_pace_engine.c - Unit tests for pace engine
 */

#include "../../src/c/pace_engine.h"
#include "../unity/unity.h"

void setUp(void) {
  // Reset pace engine before each test
  pace_engine_init(300); // 5:00/km target
}

void tearDown(void) {
  // Nothing to clean up
}

// Test pace_calculate_state pure function
void test_pace_on_target_when_within_tolerance(void) {
  // Within ±10 seconds tolerance
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_calculate_state(0));
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_calculate_state(5));
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_calculate_state(-5));
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_calculate_state(10));
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_calculate_state(-10));
}

void test_pace_ahead_when_faster_than_target(void) {
  // Negative delta means running faster
  TEST_ASSERT_EQUAL(PACE_AHEAD, pace_calculate_state(-15));
  TEST_ASSERT_EQUAL(PACE_AHEAD, pace_calculate_state(-30));
  TEST_ASSERT_EQUAL(PACE_AHEAD, pace_calculate_state(-60));
}

void test_pace_behind_when_slower_than_target(void) {
  // Positive delta between tolerance and danger zone
  TEST_ASSERT_EQUAL(PACE_BEHIND, pace_calculate_state(15));
  TEST_ASSERT_EQUAL(PACE_BEHIND, pace_calculate_state(20));
  TEST_ASSERT_EQUAL(PACE_BEHIND, pace_calculate_state(30));
}

void test_pace_danger_when_critically_behind(void) {
  // More than 30 seconds behind
  TEST_ASSERT_EQUAL(PACE_DANGER, pace_calculate_state(35));
  TEST_ASSERT_EQUAL(PACE_DANGER, pace_calculate_state(60));
  TEST_ASSERT_EQUAL(PACE_DANGER, pace_calculate_state(120));
}

// Test pace_engine_update
void test_update_returns_correct_state(void) {
  // Target is 300 (5:00/km)
  TEST_ASSERT_EQUAL(PACE_ON_TARGET, pace_engine_update(300)); // Exact
  TEST_ASSERT_EQUAL(PACE_AHEAD, pace_engine_update(280));     // Faster
  TEST_ASSERT_EQUAL(PACE_BEHIND, pace_engine_update(315));    // Slower
  TEST_ASSERT_EQUAL(PACE_DANGER, pace_engine_update(340));    // Way behind
}

// Test distance from killer
void test_distance_decreases_when_behind(void) {
  const PaceData *data = pace_engine_get_data();
  int32_t initial = data->distance_from_killer;

  // Run slower than target multiple times
  pace_engine_update(320); // 20 sec behind
  TEST_ASSERT_TRUE(pace_engine_get_data()->distance_from_killer < initial);
}

void test_distance_increases_when_ahead(void) {
  const PaceData *data = pace_engine_get_data();
  int32_t initial = data->distance_from_killer;

  // Run faster than target
  pace_engine_update(280); // 20 sec ahead
  TEST_ASSERT_TRUE(pace_engine_get_data()->distance_from_killer > initial);
}

void test_distance_holds_steady_when_on_target(void) {
  const PaceData *data = pace_engine_get_data();
  int32_t initial = data->distance_from_killer;

  // Small deltas within tolerance (<= PACE_TOLERANCE_SOFT) must not move the
  // lead, so a well-paced runner's bar doesn't slowly drift.
  pace_engine_update(305); // +5 behind, within tolerance
  TEST_ASSERT_EQUAL(initial, pace_engine_get_data()->distance_from_killer);
  pace_engine_update(295); // -5 ahead, within tolerance
  TEST_ASSERT_EQUAL(initial, pace_engine_get_data()->distance_from_killer);
  pace_engine_update(310); // +10, boundary of tolerance
  TEST_ASSERT_EQUAL(initial, pace_engine_get_data()->distance_from_killer);
}

void test_distance_capped_at_zero(void) {
  // Run way behind multiple times
  for (int i = 0; i < 100; i++) {
    pace_engine_update(400);
  }
  TEST_ASSERT_EQUAL(0, pace_engine_get_data()->distance_from_killer);
}

void test_distance_capped_at_max(void) {
  // Run way ahead multiple times
  for (int i = 0; i < 100; i++) {
    pace_engine_update(200);
  }
  TEST_ASSERT_EQUAL(200, pace_engine_get_data()->distance_from_killer);
}

// Test target adjustment
void test_adjust_target_decreases(void) {
  pace_engine_adjust_target(-30);
  TEST_ASSERT_EQUAL(270, pace_engine_get_data()->target_pace_sec_per_km);
}

void test_adjust_target_increases(void) {
  pace_engine_adjust_target(30);
  TEST_ASSERT_EQUAL(330, pace_engine_get_data()->target_pace_sec_per_km);
}

void test_target_clamped_to_minimum(void) {
  pace_engine_init(200);
  pace_engine_adjust_target(-100); // Try to go below 3:00/km
  TEST_ASSERT_EQUAL(180, pace_engine_get_data()->target_pace_sec_per_km);
}

void test_target_clamped_to_maximum(void) {
  pace_engine_init(850);
  pace_engine_adjust_target(100); // Try to go above 15:00/km
  TEST_ASSERT_EQUAL(900, pace_engine_get_data()->target_pace_sec_per_km);
}

// Test reset
void test_reset_restores_initial_distance(void) {
  pace_engine_update(400);
  pace_engine_reset();
  TEST_ASSERT_EQUAL(100, pace_engine_get_data()->distance_from_killer);
}

void test_set_target_preserves_current_pace(void) {
  pace_engine_update(320);
  pace_engine_set_target(280);
  TEST_ASSERT_EQUAL(320, pace_engine_get_data()->current_pace_sec_per_km);
  TEST_ASSERT_EQUAL(280, pace_engine_get_data()->target_pace_sec_per_km);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pace_on_target_when_within_tolerance);
  RUN_TEST(test_pace_ahead_when_faster_than_target);
  RUN_TEST(test_pace_behind_when_slower_than_target);
  RUN_TEST(test_pace_danger_when_critically_behind);
  RUN_TEST(test_update_returns_correct_state);
  RUN_TEST(test_distance_decreases_when_behind);
  RUN_TEST(test_distance_increases_when_ahead);
  RUN_TEST(test_distance_holds_steady_when_on_target);
  RUN_TEST(test_distance_capped_at_zero);
  RUN_TEST(test_distance_capped_at_max);
  RUN_TEST(test_adjust_target_decreases);
  RUN_TEST(test_adjust_target_increases);
  RUN_TEST(test_target_clamped_to_minimum);
  RUN_TEST(test_target_clamped_to_maximum);
  RUN_TEST(test_reset_restores_initial_distance);
  RUN_TEST(test_set_target_preserves_current_pace);

  return UNITY_END();
}
