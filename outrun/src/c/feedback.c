/**
 * feedback.c - Event-to-pattern routing through the active watch (pure).
 */

#include "feedback.h"
#include "watch_interface.h"

#include <stddef.h>

void feedback_fire(HapticEvent event) {
  const HapticPattern *pattern = haptic_pattern_for(event);
  if (!pattern) {
    return;
  }
  const WatchInterface *w = watch();
  if (w && w->play) {
    w->play(pattern);
  }
}

void feedback_cancel(void) {
  const WatchInterface *w = watch();
  if (w && w->cancel) {
    w->cancel();
  }
}
