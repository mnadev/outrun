/**
 * pebble.h - Stub Pebble SDK header for off-target unit testing
 *
 * This provides minimal type definitions needed to compile and test
 * our C code on the host machine without the real Pebble SDK.
 */

#ifndef PEBBLE_H
#define PEBBLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Basic types
typedef void *Window;
typedef void *Layer;
typedef void *TextLayer;
typedef void *AppTimer;
typedef void *ClickRecognizerRef;

// GRect and GColor stubs
typedef struct {
  int16_t x, y;
} GPoint;

typedef struct {
  int16_t w, h;
} GSize;

typedef struct {
  GPoint origin;
  GSize size;
} GRect;

#define GRect(x, y, w, h) ((GRect){{(x), (y)}, {(w), (h)}})

typedef uint8_t GColor;
#define GColorBlack 0
#define GColorWhite 1

// Text alignment
typedef enum {
  GTextAlignmentLeft,
  GTextAlignmentCenter,
  GTextAlignmentRight
} GTextAlignment;

// Button IDs
typedef enum {
  BUTTON_ID_BACK = 0,
  BUTTON_ID_UP,
  BUTTON_ID_SELECT,
  BUTTON_ID_DOWN,
  NUM_BUTTONS
} ButtonId;

// Log levels
typedef enum {
  APP_LOG_LEVEL_ERROR = 1,
  APP_LOG_LEVEL_WARNING = 50,
  APP_LOG_LEVEL_INFO = 100,
  APP_LOG_LEVEL_DEBUG = 200,
  APP_LOG_LEVEL_DEBUG_VERBOSE = 255
} AppLogLevel;

// Stubs for Pebble functions (do nothing in tests)
#define APP_LOG(level, fmt, ...) ((void)0)
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

// Window handlers
typedef struct {
  void (*load)(Window *window);
  void (*unload)(Window *window);
} WindowHandlers;

// Vibe pattern
typedef struct {
  const uint32_t *durations;
  uint32_t num_segments;
} VibePattern;

// Font key stubs
#define FONT_KEY_GOTHIC_14_BOLD "gothic14bold"
#define FONT_KEY_GOTHIC_18_BOLD "gothic18bold"
#define FONT_KEY_GOTHIC_24_BOLD "gothic24bold"
#define FONT_KEY_LECO_42_NUMBERS "leco42numbers"

// Stub function declarations (implemented as no-ops or return values for
// testing)
static inline Window *window_create(void) { return NULL; }
static inline void window_destroy(Window *w) { (void)w; }
static inline void window_stack_push(Window *w, bool animated) {
  (void)w;
  (void)animated;
}
static inline void window_set_click_config_provider(Window *w, void *p) {
  (void)w;
  (void)p;
}
static inline void window_set_window_handlers(Window *w, WindowHandlers h) {
  (void)w;
  (void)h;
}
static inline void window_set_background_color(Window *w, GColor c) {
  (void)w;
  (void)c;
}
static inline Layer *window_get_root_layer(Window *w) {
  (void)w;
  return NULL;
}
static inline GRect layer_get_bounds(Layer *l) {
  (void)l;
  return GRect(0, 0, 144, 168);
}
static inline void layer_add_child(Layer *p, Layer *c) {
  (void)p;
  (void)c;
}
static inline void layer_set_hidden(Layer *l, bool h) {
  (void)l;
  (void)h;
}

static inline TextLayer *text_layer_create(GRect r) {
  (void)r;
  return NULL;
}
static inline void text_layer_destroy(TextLayer *t) { (void)t; }
static inline void text_layer_set_text(TextLayer *t, const char *s) {
  (void)t;
  (void)s;
}
static inline void text_layer_set_font(TextLayer *t, void *f) {
  (void)t;
  (void)f;
}
static inline void text_layer_set_text_alignment(TextLayer *t,
                                                 GTextAlignment a) {
  (void)t;
  (void)a;
}
static inline void text_layer_set_background_color(TextLayer *t, GColor c) {
  (void)t;
  (void)c;
}
static inline void text_layer_set_text_color(TextLayer *t, GColor c) {
  (void)t;
  (void)c;
}
static inline Layer *text_layer_get_layer(TextLayer *t) {
  (void)t;
  return NULL;
}

static inline void *fonts_get_system_font(const char *key) {
  (void)key;
  return NULL;
}

static inline void window_single_click_subscribe(ButtonId id, void *h) {
  (void)id;
  (void)h;
}
static inline void window_long_click_subscribe(ButtonId id, uint16_t delay,
                                               void *down, void *up) {
  (void)id;
  (void)delay;
  (void)down;
  (void)up;
}

// Vibration stubs - track calls for testing
static int vibes_call_count = 0;
static inline void vibes_short_pulse(void) { vibes_call_count++; }
static inline void vibes_double_pulse(void) { vibes_call_count++; }
static inline void vibes_long_pulse(void) { vibes_call_count++; }
static inline void vibes_enqueue_custom_pattern(VibePattern p) {
  (void)p;
  vibes_call_count++;
}
static inline void vibes_cancel(void) { vibes_call_count = 0; }

// Timer stubs
static inline AppTimer *app_timer_register(uint32_t ms, void *cb, void *data) {
  (void)ms;
  (void)cb;
  (void)data;
  return NULL;
}
static inline void app_timer_cancel(AppTimer *t) { (void)t; }

// Event loop stub
static inline void app_event_loop(void) {}

// Persistent storage (in-memory stub implemented in support/persist_stub.c).
bool persist_exists(uint32_t key);
int persist_read_data(uint32_t key, void *buffer, size_t length);
int persist_write_data(uint32_t key, const void *data, size_t length);

#endif // PEBBLE_H
