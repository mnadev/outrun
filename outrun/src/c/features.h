/**
 * features.h - Feature flags for gating premium/connected features
 *
 * All features default to standalone mode. When companion app connects,
 * it can enable connected features. Premium features require subscription.
 */

#pragma once

#include <stdbool.h>

// Feature categories
typedef enum {
  FEATURE_STRAVA_SYNC,     // Strava OAuth + segments
  FEATURE_LEAGUES,         // Team/league system
  FEATURE_GHOST_RACING,    // Race against friend's GPS (premium)
  FEATURE_CUSTOM_STALKERS, // Custom vibration themes (premium)
  FEATURE_ANALYTICS,       // Fear Factor analytics (premium)
  FEATURE_COUNT
} Feature;

/**
 * Initialize feature flags (all disabled by default for standalone mode).
 */
void features_init(void);

/**
 * Check if a feature is enabled.
 */
bool feature_is_enabled(Feature feature);

/**
 * Enable a feature (called when companion app connects or premium unlocked).
 */
void feature_enable(Feature feature);

/**
 * Disable a feature.
 */
void feature_disable(Feature feature);

/**
 * Check if we have a phone connection.
 */
bool features_is_connected(void);

/**
 * Set connection status (called from AppMessage handlers).
 */
void features_set_connected(bool connected);
