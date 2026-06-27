/**
 * accent.c - Map the AccentColor setting to a GColor.
 *
 * Color constants beyond black/white only exist on PBL_COLOR builds, so the
 * whole palette is guarded; black-and-white watches always get white.
 */

#include "accent.h"

#include "settings.h"
#include "stalker_themes.h"

GColor accent_get_color(void) {
#if defined(PBL_COLOR)
  switch ((AccentColor)settings_get()->accent) {
  case ACCENT_RED:
    return GColorRed;
  case ACCENT_ORANGE:
    return GColorOrange;
  case ACCENT_YELLOW:
    return GColorYellow;
  case ACCENT_GREEN:
    return GColorGreen;
  case ACCENT_CYAN:
    return GColorCyan;
  case ACCENT_BLUE:
    return GColorBlue;
  case ACCENT_MAGENTA:
    return GColorMagenta;
  case ACCENT_WHITE:
    return GColorWhite;
  case ACCENT_THEME:
  default:
    return themes_get_primary_color(themes_get_current());
  }
#else
  return GColorWhite;
#endif
}
