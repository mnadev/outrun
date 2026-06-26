/**
 * watch.c - Holds the active WatchInterface and null-safe accessors (pure).
 */

#include "watch_interface.h"

#include <stddef.h>

static const WatchInterface *s_watch = NULL;

void watch_set(const WatchInterface *impl) { s_watch = impl; }

const WatchInterface *watch(void) { return s_watch; }

uint32_t watch_now_seconds(void) {
  if (s_watch && s_watch->now_seconds) {
    return s_watch->now_seconds();
  }
  return 0;
}

uint8_t watch_heart_rate(void) {
  if (s_watch && s_watch->heart_rate) {
    return s_watch->heart_rate();
  }
  return 0;
}

bool watch_heart_rate_available(void) {
  if (s_watch && s_watch->heart_rate_available) {
    return s_watch->heart_rate_available();
  }
  return false;
}
