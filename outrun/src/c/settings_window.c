/**
 * settings_window.c - Units, pace target, HR zone, alert toggles
 */

#include "settings_window.h"
#include "settings.h"

static Window *s_window;
static MenuLayer *s_menu;

typedef enum {
  SETTING_UNITS = 0,
  SETTING_TARGET_PACE,
  SETTING_HR_ZONE,
  SETTING_PACE_ALERTS,
  SETTING_HR_ALERTS,
  SETTING_COUNT
} SettingItem;

static uint16_t menu_get_num_rows(MenuLayer *menu, uint16_t section_index, void *data) {
  (void)menu;
  (void)section_index;
  (void)data;
  return SETTING_COUNT;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                          void *data) {
  const AppSettings *settings = settings_get();
  char subtitle[24];
  const char *title = "";

  switch (cell_index->row) {
  case SETTING_UNITS:
    title = "Units";
    snprintf(subtitle, sizeof(subtitle), "%s",
             settings->units == UNITS_KM ? "km" : "mi");
    break;
  case SETTING_TARGET_PACE:
    title = "Target Pace";
    settings_format_pace(subtitle, sizeof(subtitle), settings->target_pace_sec_per_km);
    break;
  case SETTING_HR_ZONE:
    title = "HR Zone";
    snprintf(subtitle, sizeof(subtitle), "%u-%u bpm", settings->hr_zone_lo,
             settings->hr_zone_hi);
    break;
  case SETTING_PACE_ALERTS:
    title = "Pace Alerts";
    snprintf(subtitle, sizeof(subtitle), "%s",
             settings->pace_alerts_enabled ? "On" : "Off");
    break;
  case SETTING_HR_ALERTS:
    title = "HR Alerts";
    snprintf(subtitle, sizeof(subtitle), "%s",
             settings->hr_alerts_enabled ? "On" : "Off");
    break;
  default:
    subtitle[0] = '\0';
    break;
  }

  graphics_context_set_text_color(ctx, GColorBlack);
  menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void menu_select_click(MenuLayer *menu, MenuIndex *cell_index, void *data) {
  const AppSettings *settings = settings_get();

  switch (cell_index->row) {
  case SETTING_UNITS:
    settings_set_units(settings->units == UNITS_KM ? UNITS_MI : UNITS_KM);
    break;
  case SETTING_TARGET_PACE:
    settings_adjust_target_pace(-15);
    break;
  case SETTING_HR_ZONE:
    settings_set_hr_zone(settings->hr_zone_lo + 5, settings->hr_zone_hi + 5);
    break;
  case SETTING_PACE_ALERTS:
    settings_set_pace_alerts(!settings->pace_alerts_enabled);
    break;
  case SETTING_HR_ALERTS:
    settings_set_hr_alerts(!settings->hr_alerts_enabled);
    break;
  default:
    break;
  }

  menu_layer_reload_data(s_menu);
}

static void menu_select_long_click(MenuLayer *menu, MenuIndex *cell_index, void *data) {
  const AppSettings *settings = settings_get();

  switch (cell_index->row) {
  case SETTING_TARGET_PACE:
    settings_adjust_target_pace(15);
    break;
  case SETTING_HR_ZONE:
    settings_set_hr_zone(settings->hr_zone_lo - 5, settings->hr_zone_hi - 5);
    break;
  default:
    break;
  }

  menu_layer_reload_data(s_menu);
}

static void click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, NULL);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu, NULL,
                           (MenuLayerCallbacks){
                               .get_num_rows = menu_get_num_rows,
                               .draw_row = menu_draw_row,
                               .select_click = menu_select_click,
                               .select_long_click = menu_select_long_click,
                           });
  menu_layer_set_click_config_onto_window(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  window_destroy(s_window);
  s_window = NULL;
}

void settings_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window,
                             (WindowHandlers){.load = window_load, .unload = window_unload});
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}
