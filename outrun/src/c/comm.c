/**
 * comm.c - Phone-Watch communication via AppMessage
 */

#include "comm.h"
#include "features.h"
#include "hr_monitor.h"
#include "plan.h"
#include "run_window.h"

enum {
  KEY_CURRENT_PACE = 0,
  KEY_TARGET_PACE = 1,
  KEY_IS_RUNNING = 2,
  KEY_COMMAND = 3,
  KEY_SEGMENT_ACTIVE = 4,
  KEY_SEGMENT_NAME = 5,
  KEY_RIVAL_NAME = 6,
  KEY_RIVAL_TIME = 7,
  KEY_CONNECTED = 8,
  KEY_CURRENT_DISTANCE = 9,
  KEY_HEART_RATE = 10,
  KEY_PLAN_INDEX = 11,
  KEY_PLAN_NAME = 12,
  KEY_PLAN_SEG_COUNT = 13,
  KEY_PLAN_DATA = 14,
  KEY_PLAN_TOTAL = 15
};

static char s_segment_name[32];
static char s_rival_name[32];
static CommPlanReceivedCallback s_plan_callback;
static uint32_t s_last_distance;

static void handle_plan_message(DictionaryIterator *iterator) {
  Tuple *total_tuple = dict_find(iterator, KEY_PLAN_TOTAL);
  if (total_tuple) {
    plan_store_clear();
    plan_store_set_expected_total((uint8_t)total_tuple->value->int32);
  }

  Tuple *index_tuple = dict_find(iterator, KEY_PLAN_INDEX);
  Tuple *name_tuple = dict_find(iterator, KEY_PLAN_NAME);
  Tuple *count_tuple = dict_find(iterator, KEY_PLAN_SEG_COUNT);
  Tuple *data_tuple = dict_find(iterator, KEY_PLAN_DATA);

  if (!index_tuple || !name_tuple || !count_tuple || !data_tuple) {
    return;
  }

  uint8_t index = (uint8_t)index_tuple->value->int32;
  uint8_t seg_count = (uint8_t)count_tuple->value->int32;
  const char *name = name_tuple->value->cstring;

  plan_store_add(index, name, seg_count, data_tuple->value->data,
                 data_tuple->length);

  if (s_plan_callback) {
    s_plan_callback();
  }
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *connected_tuple = dict_find(iterator, KEY_CONNECTED);
  if (connected_tuple) {
    features_set_connected(connected_tuple->value->int32 == 1);
  }

  Tuple *pace_tuple = dict_find(iterator, KEY_CURRENT_PACE);
  if (pace_tuple) {
    pace_engine_update(pace_tuple->value->int32);
    run_window_update();
  }

  Tuple *target_tuple = dict_find(iterator, KEY_TARGET_PACE);
  if (target_tuple) {
    pace_engine_init(target_tuple->value->int32);
  }

  Tuple *distance_tuple = dict_find(iterator, KEY_CURRENT_DISTANCE);
  if (distance_tuple) {
    uint32_t distance = (uint32_t)distance_tuple->value->int32;
    if (distance >= s_last_distance) {
      run_state_set_distance(distance);
    }
    s_last_distance = distance;
    run_window_update();
  }

  Tuple *hr_tuple = dict_find(iterator, KEY_HEART_RATE);
  if (hr_tuple) {
    hr_monitor_set_debug_bpm((uint8_t)hr_tuple->value->int32);
    run_window_update();
  }

  Tuple *plan_tuple = dict_find(iterator, KEY_PLAN_INDEX);
  if (plan_tuple) {
    handle_plan_message(iterator);
  }

  Tuple *segment_active_tuple = dict_find(iterator, KEY_SEGMENT_ACTIVE);
  if (segment_active_tuple) {
    if (segment_active_tuple->value->int32 == 1) {
      Tuple *segment_name_tuple = dict_find(iterator, KEY_SEGMENT_NAME);
      Tuple *rival_name_tuple = dict_find(iterator, KEY_RIVAL_NAME);

      if (segment_name_tuple) {
        strncpy(s_segment_name, segment_name_tuple->value->cstring,
                sizeof(s_segment_name) - 1);
      } else {
        strncpy(s_segment_name, "Unknown", sizeof(s_segment_name) - 1);
      }

      if (rival_name_tuple) {
        strncpy(s_rival_name, rival_name_tuple->value->cstring,
                sizeof(s_rival_name) - 1);
      } else {
        strncpy(s_rival_name, "Rival", sizeof(s_rival_name) - 1);
      }

      run_window_show_segment_alert(s_segment_name, s_rival_name);
    } else {
      run_window_hide_segment_alert();
    }
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage dropped: %d", reason);
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {}

static void outbox_failed_handler(DictionaryIterator *iterator,
                                  AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage send failed: %d", reason);
}

void comm_init(void) {
  s_plan_callback = NULL;
  s_last_distance = 0;

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);

  const int inbox_size = 512;
  const int outbox_size = 64;
  app_message_open(inbox_size, outbox_size);
}

void comm_deinit(void) { app_message_deregister_callbacks(); }

void comm_send_command(int command) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_COMMAND, command);
  app_message_outbox_send();
}

void comm_send_target_pace(int32_t pace) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_TARGET_PACE, pace);
  app_message_outbox_send();
}

void comm_set_plan_received_callback(CommPlanReceivedCallback callback) {
  s_plan_callback = callback;
}
