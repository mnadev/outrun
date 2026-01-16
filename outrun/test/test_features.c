/**
 * test_features.c - Unit tests for feature flags
 */

#include "../../src/c/features.h"
#include "../unity/unity.h"

void setUp(void) { features_init(); }

void tearDown(void) {
  // Nothing to clean up
}

// Test initial state
void test_all_features_disabled_initially(void) {
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_STRAVA_SYNC));
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_LEAGUES));
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_GHOST_RACING));
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_CUSTOM_STALKERS));
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_ANALYTICS));
}

void test_not_connected_initially(void) {
  TEST_ASSERT_FALSE(features_is_connected());
}

// Test enable/disable
void test_enable_feature(void) {
  feature_enable(FEATURE_STRAVA_SYNC);
  TEST_ASSERT_TRUE(feature_is_enabled(FEATURE_STRAVA_SYNC));
}

void test_disable_feature(void) {
  feature_enable(FEATURE_LEAGUES);
  feature_disable(FEATURE_LEAGUES);
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_LEAGUES));
}

void test_enable_multiple_features(void) {
  feature_enable(FEATURE_GHOST_RACING);
  feature_enable(FEATURE_CUSTOM_STALKERS);
  feature_enable(FEATURE_ANALYTICS);

  TEST_ASSERT_TRUE(feature_is_enabled(FEATURE_GHOST_RACING));
  TEST_ASSERT_TRUE(feature_is_enabled(FEATURE_CUSTOM_STALKERS));
  TEST_ASSERT_TRUE(feature_is_enabled(FEATURE_ANALYTICS));
}

// Test connection status
void test_set_connected(void) {
  features_set_connected(true);
  TEST_ASSERT_TRUE(features_is_connected());
}

void test_disconnect_disables_connected_features(void) {
  feature_enable(FEATURE_STRAVA_SYNC);
  feature_enable(FEATURE_LEAGUES);
  feature_enable(FEATURE_GHOST_RACING); // Premium, not connected

  features_set_connected(false);

  // Connected features should be disabled
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_STRAVA_SYNC));
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_LEAGUES));

  // Premium features unaffected
  TEST_ASSERT_TRUE(feature_is_enabled(FEATURE_GHOST_RACING));
}

// Test invalid feature handling
void test_invalid_feature_returns_false(void) {
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_COUNT)); // Out of bounds
  TEST_ASSERT_FALSE(feature_is_enabled(FEATURE_COUNT + 1));
}

int main(void) {
  UNITY_BEGIN();

  // Initial state
  RUN_TEST(test_all_features_disabled_initially);
  RUN_TEST(test_not_connected_initially);

  // Enable/disable
  RUN_TEST(test_enable_feature);
  RUN_TEST(test_disable_feature);
  RUN_TEST(test_enable_multiple_features);

  // Connection
  RUN_TEST(test_set_connected);
  RUN_TEST(test_disconnect_disables_connected_features);

  // Invalid input
  RUN_TEST(test_invalid_feature_returns_false);

  return UNITY_END();
}
