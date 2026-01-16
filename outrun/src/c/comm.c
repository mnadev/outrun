/**
 * comm.c - Phone-Watch communication via AppMessage
 *
 * Handles receiving pace data from the phone and sending commands.
 */

#include "comm.h"
#include "features.h"
#include "run_window.h"

// Message keys (must match package.json order)
enum {
  KEY_CURRENT_PACE = 0,
  KEY_TARGET_PACE = 1,
  KEY_IS_RUNNING = 2,
  KEY_COMMAND = 3,
  KEY_SEGMENT_ACTIVE = 4,
  KEY_SEGMENT_NAME = 5,
  KEY_RIVAL_NAME = 6,
  KEY_RIVAL_TIME = 7,
  KEY_CONNECTED = 8
};

// Buffers for segment/rival names
static char s_segment_name[32];
static char s_rival_name[32];

/**
 * Handle incoming AppMessage from phone
 */
static void inbox_received_handler(DictionaryIterator *iterator,
                                   void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage received");

  Tuple *connected_tuple = dict_find(iterator, KEY_CONNECTED);
  if (connected_tuple) {
    features_set_connected(connected_tuple->value->int32 == 1);
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone connected!");
  }

  Tuple *pace_tuple = dict_find(iterator, KEY_CURRENT_PACE);
  if (pace_tuple) {
    int32_t current_pace = pace_tuple->value->int32;
    pace_engine_update(current_pace);
    run_window_update();
  }

  Tuple *target_tuple = dict_find(iterator, KEY_TARGET_PACE);
  if (target_tuple) {
    // Target pace updated from phone (e.g., from config)
    int32_t target_pace = target_tuple->value->int32;
    pace_engine_init(target_pace);
  }

  Tuple *segment_active_tuple = dict_find(iterator, KEY_SEGMENT_ACTIVE);
  if (segment_active_tuple) {
    if (segment_active_tuple->value->int32 == 1) {
      // Get segment and rival names
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

/**
 * Handle AppMessage dropped
 */
static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage dropped: %d", reason);
}

/**
 * Handle outbox sent
 */
static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage sent successfully");
}

/**
 * Handle outbox failed
 */
static void outbox_failed_handler(DictionaryIterator *iterator,
                                  AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage send failed: %d", reason);
}

void comm_init(void) {
  // Register handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);

  // Open AppMessage with recommended buffer sizes
  const int inbox_size = 256;
  const int outbox_size = 64;
  app_message_open(inbox_size, outbox_size);

  APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage initialized");
}

void comm_deinit(void) { app_message_deregister_callbacks(); }

void comm_send_command(int command) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin outbox: %d", result);
    return;
  }

  dict_write_int32(iter, KEY_COMMAND, command);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send command: %d", result);
  }
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
