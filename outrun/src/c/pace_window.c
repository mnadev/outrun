/**
 * pace_window.c - Target-pace picker styled after the Pebble alarm screen.
 *
 * Two dials (minutes : seconds) shown in the user's display unit. UP/DOWN spin
 * the highlighted dial (hold to repeat), SELECT advances minutes -> seconds ->
 * save, BACK cancels. The edited value is kept in display seconds and clamped
 * to the valid pace range, then converted back to sec/km on save.
 */

#include "pace_window.h"

#include "settings.h"
#include "stalker_themes.h"

#include <pebble.h>

#define SEC_STEP 5

typedef enum { FIELD_MIN = 0, FIELD_SEC = 1 } PaceField;

static Window *s_window;
static Layer *s_layer;
static GPath *s_chevron_up;
static GPath *s_chevron_down;

static int32_t s_total;     // edited value, seconds in the display unit
static int32_t s_min_total; // clamp bounds (display seconds)
static int32_t s_max_total;
static PaceField s_field;

static const GPathInfo CHEVRON_UP_INFO = {
    .num_points = 3, .points = (GPoint[]){{-9, 5}, {0, -5}, {9, 5}}};
static const GPathInfo CHEVRON_DOWN_INFO = {
    .num_points = 3, .points = (GPoint[]){{-9, -5}, {0, 5}, {9, -5}}};

static int32_t round_to_step(int32_t value, int32_t step) {
  return ((value + step / 2) / step) * step;
}

static void clamp_total(void) {
  if (s_total < s_min_total) {
    s_total = s_min_total;
  }
  if (s_total > s_max_total) {
    s_total = s_max_total;
  }
}

static void adjust(int dir) {
  s_total += dir * (s_field == FIELD_MIN ? 60 : SEC_STEP);
  clamp_total();
  layer_mark_dirty(s_layer);
}

static GColor highlight_color(void) {
#if defined(PBL_COLOR)
  return themes_get_primary_color(themes_get_current());
#else
  return GColorWhite;
#endif
}

static void draw_field(GContext *ctx, GRect box, const char *txt, bool selected,
                       GColor hi) {
  GColor text_color = GColorWhite;
  if (selected) {
    graphics_context_set_fill_color(ctx, hi);
    graphics_fill_rect(ctx, box, 6, GCornersAll);
    text_color = GColorBlack;
  }
  graphics_context_set_text_color(ctx, text_color);
  GRect tb = GRect(box.origin.x, box.origin.y + (box.size.h - 42) / 2 - 2,
                   box.size.w, 44);
  graphics_draw_text(ctx, txt, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                     tb, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void layer_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);

  const int fw = 54;
  const int cw = 18;
  const int boxh = 50;
  const int total_w = fw * 2 + cw;
  const int x0 = (b.size.w - total_w) / 2;
  const int row_y = b.size.h / 2 - boxh / 2 - 6;

  GRect min_box = GRect(x0, row_y, fw, boxh);
  GRect colon_box = GRect(x0 + fw, row_y, cw, boxh);
  GRect sec_box = GRect(x0 + fw + cw, row_y, fw, boxh);

  GColor hi = highlight_color();

  // Title.
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, "TARGET PACE",
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, row_y - 30, b.size.w, 22), GTextOverflowModeFill,
                     GTextAlignmentCenter, NULL);

  // MM : SS
  char mm[4];
  char ss[4];
  snprintf(mm, sizeof(mm), "%02d", (int)(s_total / 60));
  snprintf(ss, sizeof(ss), "%02d", (int)(s_total % 60));

  draw_field(ctx, min_box, mm, s_field == FIELD_MIN, hi);
  draw_field(ctx, sec_box, ss, s_field == FIELD_SEC, hi);

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, ":", fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
                     GRect(colon_box.origin.x, colon_box.origin.y - 4,
                           colon_box.size.w, 44),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // Up/down chevrons over the selected dial.
  GRect sel = (s_field == FIELD_MIN) ? min_box : sec_box;
  int cx = sel.origin.x + sel.size.w / 2;
  graphics_context_set_fill_color(ctx, hi);
  gpath_move_to(s_chevron_up, GPoint(cx, row_y - 11));
  gpath_draw_filled(ctx, s_chevron_up);
  gpath_move_to(s_chevron_down, GPoint(cx, row_y + boxh + 11));
  gpath_draw_filled(ctx, s_chevron_down);

  // Unit + hint.
  const AppSettings *settings = settings_get();
  const char *unit = (settings->units == UNITS_MI) ? "min / mi" : "min / km";
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, unit, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, row_y + boxh + 18, b.size.w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  const char *hint = (s_field == FIELD_MIN) ? "SELECT: seconds" : "SELECT: save";
  graphics_draw_text(ctx, hint, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, row_y + boxh + 36, b.size.w, 18),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  adjust(+1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  adjust(-1);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (s_field == FIELD_MIN) {
    s_field = FIELD_SEC;
    layer_mark_dirty(s_layer);
    return;
  }
  settings_set_target_pace(settings_pace_from_display(s_total));
  window_stack_pop(true);
}

static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100,
                                          down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  window_set_background_color(window, GColorBlack);

  s_chevron_up = gpath_create(&CHEVRON_UP_INFO);
  s_chevron_down = gpath_create(&CHEVRON_DOWN_INFO);

  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, layer_update);
  layer_add_child(root, s_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
  gpath_destroy(s_chevron_up);
  gpath_destroy(s_chevron_down);
  s_layer = NULL;
  s_chevron_up = NULL;
  s_chevron_down = NULL;
  window_destroy(s_window);
  s_window = NULL;
}

void pace_window_push(void) {
  // Valid pace range in display seconds, snapped to the dial step.
  s_min_total = round_to_step(settings_display_pace(180) + SEC_STEP - 1, SEC_STEP);
  s_max_total = (settings_display_pace(900) / SEC_STEP) * SEC_STEP;

  int32_t current = settings_display_pace(settings_get()->target_pace_sec_per_km);
  s_total = round_to_step(current, SEC_STEP);
  clamp_total();
  s_field = FIELD_MIN;

  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(
      s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
}
