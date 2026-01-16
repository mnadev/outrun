/**
 * features.c - Feature flags implementation
 */

#include "features.h"

static bool s_features[FEATURE_COUNT];
static bool s_connected = false;

void features_init(void) {
  // All features disabled by default (standalone mode)
  for (int i = 0; i < FEATURE_COUNT; i++) {
    s_features[i] = false;
  }
  s_connected = false;
}

bool feature_is_enabled(Feature feature) {
  if (feature >= FEATURE_COUNT) {
    return false;
  }
  return s_features[feature];
}

void feature_enable(Feature feature) {
  if (feature < FEATURE_COUNT) {
    s_features[feature] = true;
  }
}

void feature_disable(Feature feature) {
  if (feature < FEATURE_COUNT) {
    s_features[feature] = false;
  }
}

bool features_is_connected(void) { return s_connected; }

void features_set_connected(bool connected) {
  s_connected = connected;

  // When disconnected, disable connected features
  if (!connected) {
    feature_disable(FEATURE_STRAVA_SYNC);
    feature_disable(FEATURE_LEAGUES);
  }
}
