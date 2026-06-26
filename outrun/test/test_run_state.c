/**
 * test_run_state.c - Unit tests for run state machine
 */

#include "../../src/c/run_state.h"
#include "../unity/unity.h"
#include "support/mock_watch.h"

void setUp(void) {
  mock_watch_install();
  mock_watch_reset();
  run_state_init();
}

void tearDown(void) { run_state_deinit(); }

// Test initial state
void test_initial_state_is_idle(void) {
  TEST_ASSERT_EQUAL(RUN_IDLE, run_state_get());
}

// Test state transitions
void test_start_from_idle(void) {
  TEST_ASSERT_TRUE(run_state_start());
  TEST_ASSERT_EQUAL(RUN_ACTIVE, run_state_get());
}

void test_cannot_start_when_active(void) {
  run_state_start();
  TEST_ASSERT_FALSE(run_state_start()); // Should fail
  TEST_ASSERT_EQUAL(RUN_ACTIVE, run_state_get());
}

void test_pause_from_active(void) {
  run_state_start();
  TEST_ASSERT_TRUE(run_state_pause());
  TEST_ASSERT_EQUAL(RUN_PAUSED, run_state_get());
}

void test_cannot_pause_when_idle(void) {
  TEST_ASSERT_FALSE(run_state_pause());
  TEST_ASSERT_EQUAL(RUN_IDLE, run_state_get());
}

void test_resume_from_paused(void) {
  run_state_start();
  run_state_pause();
  TEST_ASSERT_TRUE(run_state_resume());
  TEST_ASSERT_EQUAL(RUN_ACTIVE, run_state_get());
}

void test_cannot_resume_when_active(void) {
  run_state_start();
  TEST_ASSERT_FALSE(run_state_resume());
  TEST_ASSERT_EQUAL(RUN_ACTIVE, run_state_get());
}

void test_stop_from_active(void) {
  run_state_start();
  TEST_ASSERT_TRUE(run_state_stop());
  TEST_ASSERT_EQUAL(RUN_COMPLETE, run_state_get());
}

void test_stop_from_paused(void) {
  run_state_start();
  run_state_pause();
  TEST_ASSERT_TRUE(run_state_stop());
  TEST_ASSERT_EQUAL(RUN_COMPLETE, run_state_get());
}

void test_cannot_stop_when_idle(void) {
  TEST_ASSERT_FALSE(run_state_stop());
  TEST_ASSERT_EQUAL(RUN_IDLE, run_state_get());
}

void test_reset_from_complete(void) {
  run_state_start();
  run_state_stop();
  run_state_reset();
  TEST_ASSERT_EQUAL(RUN_IDLE, run_state_get());
}

// Test statistics
void test_stats_reset_on_start(void) {
  run_state_add_distance(1000);
  run_state_tick();
  run_state_start(); // Should reset stats

  const RunStats *stats = run_state_get_stats();
  TEST_ASSERT_EQUAL(0, stats->elapsed_seconds);
  TEST_ASSERT_EQUAL(0, stats->distance_meters);
}

void test_tick_increments_elapsed_when_active(void) {
  run_state_start();
  mock_watch_advance(1);
  run_state_tick();
  mock_watch_advance(1);
  run_state_tick();
  mock_watch_advance(1);
  run_state_tick();

  TEST_ASSERT_EQUAL(3, run_state_get_stats()->elapsed_seconds);
}

void test_tick_does_not_increment_when_paused(void) {
  run_state_start();
  mock_watch_advance(1);
  run_state_tick();
  run_state_pause();
  mock_watch_advance(1);
  run_state_tick();
  mock_watch_advance(1);
  run_state_tick();

  TEST_ASSERT_EQUAL(1, run_state_get_stats()->elapsed_seconds);
}

void test_add_distance_accumulates(void) {
  run_state_add_distance(100);
  run_state_add_distance(200);
  run_state_add_distance(300);

  TEST_ASSERT_EQUAL(600, run_state_get_stats()->distance_meters);
}

void test_avg_pace_calculated_correctly(void) {
  run_state_start();
  run_state_add_distance(1000);

  for (int i = 0; i < 300; i++) {
    mock_watch_advance(1);
    run_state_tick();
  }

  TEST_ASSERT_EQUAL(300, run_state_get_stats()->avg_pace_sec_per_km);
}

int main(void) {
  UNITY_BEGIN();

  // Initial state
  RUN_TEST(test_initial_state_is_idle);

  // State transitions
  RUN_TEST(test_start_from_idle);
  RUN_TEST(test_cannot_start_when_active);
  RUN_TEST(test_pause_from_active);
  RUN_TEST(test_cannot_pause_when_idle);
  RUN_TEST(test_resume_from_paused);
  RUN_TEST(test_cannot_resume_when_active);
  RUN_TEST(test_stop_from_active);
  RUN_TEST(test_stop_from_paused);
  RUN_TEST(test_cannot_stop_when_idle);
  RUN_TEST(test_reset_from_complete);

  // Statistics
  RUN_TEST(test_stats_reset_on_start);
  RUN_TEST(test_tick_increments_elapsed_when_active);
  RUN_TEST(test_tick_does_not_increment_when_paused);
  RUN_TEST(test_add_distance_accumulates);
  RUN_TEST(test_avg_pace_calculated_correctly);

  return UNITY_END();
}
