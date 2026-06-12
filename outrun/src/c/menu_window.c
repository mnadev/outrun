/**
 * menu_window.c - Main menu: Quick Run / Plans / Settings
 */

#include "menu_window.h"
#include "plans_window.h"
#include "run_window.h"
#include "settings_window.h"

static Window *s_window;
static MenuLayer *s_menu;

typedef enum {
  MENU_QUICK_RUN = 0,
  MENU_PLANS,
  MENU_SETTINGS,
  MENU_COUNT
} MenuItem;

static uint16_t menu_get_num_rows(MenuLayer *menu, uint16_t section_index, void *data) {
  (void)menu;
  (void)section_index;
  (void)data;
  return MENU_COUNT;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                          void *data) {
  const char *title = NULL;
  switch (cell_index->row) {
  case MENU_QUICK_RUN:
    title = "Quick Run";
    break;
  case MENU_PLANS:
    title = "Plans";
    break;
  case MENU_SETTINGS:
    title = "Settings";
    break;
  default:
    title = "";
    break;
  }
  graphics_context_set_text_color(ctx, GColorBlack);
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static void menu_select_click(MenuLayer *menu, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
  case MENU_QUICK_RUN:
    run_window_push_quick();
    break;
  case MENU_PLANS:
    plans_window_push();
    break;
  case MENU_SETTINGS:
    settings_window_push();
    break;
  default:
    break;
  }
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
                           });
  menu_layer_set_click_config_onto_window(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  window_destroy(s_window);
  s_window = NULL;
}

void menu_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window,
                             (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
}
