/**
 * plans_window.c - Select a running plan preset
 */

#include "plans_window.h"
#include "accent.h"
#include "plan.h"
#include "run_window.h"
#include "stalker_themes.h"

static Window *s_window;
static MenuLayer *s_menu;

static uint16_t menu_get_num_rows(MenuLayer *menu, uint16_t section_index, void *data) {
  (void)menu;
  (void)section_index;
  (void)data;
  uint8_t count = plan_store_count();
  return count > 0 ? count : 1;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                          void *data) {
  const RunPlan *plan = plan_store_get(cell_index->row);
  const char *title = plan ? plan->name : "No plans yet";
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static void menu_select_click(MenuLayer *menu, MenuIndex *cell_index, void *data) {
  if (plan_store_get(cell_index->row)) {
    run_window_push_planned((uint8_t)cell_index->row);
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
#if defined(PBL_ROUND)
  menu_layer_set_center_focused(s_menu, true);
#endif
#if defined(PBL_COLOR)
  menu_layer_set_highlight_colors(s_menu, accent_get_color(), GColorBlack);
#endif
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  s_menu = NULL;
  window_destroy(s_window);
  s_window = NULL;
}

void plans_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window,
                             (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
}

void plans_window_reload(void) {
  if (s_menu) {
    menu_layer_reload_data(s_menu);
  }
}
