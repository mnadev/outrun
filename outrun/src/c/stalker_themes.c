/**
 * stalker_themes.c - Custom stalker themes implementation
 */

#include "stalker_themes.h"
#include "features.h"
#include "watch_interface.h"

// Persistent storage key (settings uses key 1).
#define THEME_PERSIST_KEY 2

// Current theme
static StalkerTheme s_current_theme = THEME_CLASSIC;

// Theme configurations (without colors - those are handled by functions)
static const ThemeConfig THEMES[THEME_COUNT] = {
    // THEME_CLASSIC - Slasher movie
    {.id = THEME_CLASSIC,
     .name = "Classic Slasher",
     .stalker_name = "THE KILLER",
     .escape_message = "ESCAPED!",
     .caught_message = "CAUGHT!",
     .behind_message = "IT'S GAINING!",
     .ahead_message = "OUTRUNNING!"},
    // THEME_PARANORMAL - Ghost/supernatural
    {.id = THEME_PARANORMAL,
     .name = "Paranormal",
     .stalker_name = "THE SPIRIT",
     .escape_message = "BANISHED!",
     .caught_message = "POSSESSED!",
     .behind_message = "IT'S HAUNTING!",
     .ahead_message = "OUTRUNNING!"},
    // THEME_ZOMBIE - Zombie apocalypse
    {.id = THEME_ZOMBIE,
     .name = "Zombie Horde",
     .stalker_name = "THE HORDE",
     .escape_message = "SURVIVED!",
     .caught_message = "INFECTED!",
     .behind_message = "THEY'RE CLOSE!",
     .ahead_message = "BREAKING FREE!"},
    // THEME_ALIEN - Sci-fi
    {.id = THEME_ALIEN,
     .name = "Alien Pursuit",
     .stalker_name = "THE XENOMORPH",
     .escape_message = "ESCAPED POD!",
     .caught_message = "CAPTURED!",
     .behind_message = "CLOSING IN!",
     .ahead_message = "EVADING!"},
    // THEME_WEREWOLF - Full moon chase
    {.id = THEME_WEREWOLF,
     .name = "Full Moon",
     .stalker_name = "THE BEAST",
     .escape_message = "DAWN BREAKS!",
     .caught_message = "MAULED!",
     .behind_message = "IT HOWLS!",
     .ahead_message = "AHEAD OF IT!"}};

// Color getters (can't use GColor in static init)
GColor themes_get_primary_color(StalkerTheme theme) {
  switch (theme) {
  case THEME_PARANORMAL:
    return GColorCyan;
  case THEME_ZOMBIE:
    return GColorGreen;
  case THEME_ALIEN:
    return GColorYellow;
  case THEME_WEREWOLF:
    return GColorLightGray;
  default:
    return GColorWhite;
  }
}

GColor themes_get_danger_color(StalkerTheme theme) {
  switch (theme) {
  case THEME_PARANORMAL:
    return GColorPurple;
  case THEME_ZOMBIE:
    return GColorDarkGray;
  case THEME_ALIEN:
    return GColorOrange;
  default:
    return GColorRed;
  }
}

// Theme-specific vibration patterns
static const uint32_t CLASSIC_PULSE[] = {50, 100, 50};
static const uint32_t PARANORMAL_PULSE[] = {100, 200, 100, 200, 100};
static const uint32_t ZOMBIE_PULSE[] = {30, 50, 30, 50, 30, 50, 30};
static const uint32_t ALIEN_PULSE[] = {150, 75, 150};
static const uint32_t WEREWOLF_PULSE[] = {200, 100, 50, 100, 200};

static const uint32_t CLASSIC_DANGER[] = {100, 50, 100, 50, 100, 50, 100};
static const uint32_t PARANORMAL_DANGER[] = {300, 100, 300, 100, 300};
static const uint32_t ZOMBIE_DANGER[] = {50, 25, 50, 25, 50, 25, 50, 25, 50};
static const uint32_t ALIEN_DANGER[] = {200, 50, 200, 50, 200, 50, 200};
static const uint32_t WEREWOLF_DANGER[] = {400, 200, 400};

static const uint32_t CLASSIC_SCARE[] = {500};
static const uint32_t PARANORMAL_SCARE[] = {100, 50, 100, 50, 500};
static const uint32_t ZOMBIE_SCARE[] = {100, 50, 100, 50, 100, 50, 300};
static const uint32_t ALIEN_SCARE[] = {300, 100, 300, 100, 300};
static const uint32_t WEREWOLF_SCARE[] = {800};

void themes_init(void) {
  s_current_theme = THEME_CLASSIC;
  if (persist_exists(THEME_PERSIST_KEY)) {
    int32_t saved = persist_read_int(THEME_PERSIST_KEY);
    if (saved >= 0 && saved < THEME_COUNT) {
      s_current_theme = (StalkerTheme)saved;
    }
  }
}

StalkerTheme themes_get_current(void) { return s_current_theme; }

bool themes_set_current(StalkerTheme theme) {
  if (theme >= THEME_COUNT) {
    return false;
  }

  s_current_theme = theme;
  persist_write_int(THEME_PERSIST_KEY, (int32_t)theme);
  return true;
}

const ThemeConfig *themes_get_config(StalkerTheme theme) {
  if (theme >= THEME_COUNT) {
    return &THEMES[THEME_CLASSIC];
  }
  return &THEMES[theme];
}

const ThemeConfig *themes_get_current_config(void) {
  return &THEMES[s_current_theme];
}

static void enqueue_pattern(const uint32_t *segments, uint32_t count) {
  const WatchInterface *w = watch();
  if (w && w->play) {
    HapticPattern pattern = {.durations = segments, .count = count};
    w->play(&pattern);
  }
}

void themes_haptic_pulse(void) {
  switch (s_current_theme) {
  case THEME_PARANORMAL:
    enqueue_pattern(PARANORMAL_PULSE, ARRAY_LENGTH(PARANORMAL_PULSE));
    break;
  case THEME_ZOMBIE:
    enqueue_pattern(ZOMBIE_PULSE, ARRAY_LENGTH(ZOMBIE_PULSE));
    break;
  case THEME_ALIEN:
    enqueue_pattern(ALIEN_PULSE, ARRAY_LENGTH(ALIEN_PULSE));
    break;
  case THEME_WEREWOLF:
    enqueue_pattern(WEREWOLF_PULSE, ARRAY_LENGTH(WEREWOLF_PULSE));
    break;
  default:
    enqueue_pattern(CLASSIC_PULSE, ARRAY_LENGTH(CLASSIC_PULSE));
    break;
  }
}

void themes_haptic_danger(void) {
  switch (s_current_theme) {
  case THEME_PARANORMAL:
    enqueue_pattern(PARANORMAL_DANGER, ARRAY_LENGTH(PARANORMAL_DANGER));
    break;
  case THEME_ZOMBIE:
    enqueue_pattern(ZOMBIE_DANGER, ARRAY_LENGTH(ZOMBIE_DANGER));
    break;
  case THEME_ALIEN:
    enqueue_pattern(ALIEN_DANGER, ARRAY_LENGTH(ALIEN_DANGER));
    break;
  case THEME_WEREWOLF:
    enqueue_pattern(WEREWOLF_DANGER, ARRAY_LENGTH(WEREWOLF_DANGER));
    break;
  default:
    enqueue_pattern(CLASSIC_DANGER, ARRAY_LENGTH(CLASSIC_DANGER));
    break;
  }
}

void themes_haptic_scare(void) {
  switch (s_current_theme) {
  case THEME_PARANORMAL:
    enqueue_pattern(PARANORMAL_SCARE, ARRAY_LENGTH(PARANORMAL_SCARE));
    break;
  case THEME_ZOMBIE:
    enqueue_pattern(ZOMBIE_SCARE, ARRAY_LENGTH(ZOMBIE_SCARE));
    break;
  case THEME_ALIEN:
    enqueue_pattern(ALIEN_SCARE, ARRAY_LENGTH(ALIEN_SCARE));
    break;
  case THEME_WEREWOLF:
    enqueue_pattern(WEREWOLF_SCARE, ARRAY_LENGTH(WEREWOLF_SCARE));
    break;
  default:
    enqueue_pattern(CLASSIC_SCARE, ARRAY_LENGTH(CLASSIC_SCARE));
    break;
  }
}
