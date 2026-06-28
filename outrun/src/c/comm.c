/**
 * comm.c - Phone-Watch communication via AppMessage
 */

#include "comm.h"
#include "features.h"
#include "hr_monitor.h"
#include "plan.h"
#include "run_session.h"
#include "run_state.h"
#include "run_window.h"

#include <pebble.h>

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
  KEY_PLAN_TOTAL = 15,
  KEY_MOVING = 16
};

static char s_segment_name[32];
static char s_rival_name[32];
static CommPlanReceivedCallback s_plan_callback;
static uint32_t s_last_distance;

// Outbound reliability: AppMessage only allows one in-flight send, and
// app_message_outbox_begin() returns busy if one is pending. Rather than
// silently dropping commands (which would desync the phone's GPS state), we
// queue them and drain on the sent/failed callbacks.
//
// Commands (start/stop/pause/resume) are a small FIFO -- order and delivery
// matter. Target pace is a single coalesced slot -- only the latest value is
// worth sending. Failed sends are retried a few times, then dropped so we
// don't transmit forever while the phone is unreachable.
#define CMD_QUEUE_MAX 6
#define OUTBOX_MAX_ATTEMPTS 3

typedef enum { OUTBOX_IDLE, OUTBOX_CMD, OUTBOX_PACE } OutboxInFlight;

static int s_cmd_queue[CMD_QUEUE_MAX];
static uint8_t s_cmd_head;
static uint8_t s_cmd_count;
static bool s_pace_pending;
static int32_t s_pace_value;
static OutboxInFlight s_in_flight;
static uint8_t s_attempts;

static void cmd_pop(void) {
  if (s_cmd_count > 0) {
    s_cmd_head = (uint8_t)((s_cmd_head + 1) % CMD_QUEUE_MAX);
    s_cmd_count--;
  }
}

static void outbox_drain(void) {
  if (s_in_flight != OUTBOX_IDLE) {
    return; // a send is in flight; the sent/failed callback will re-drain
  }
  if (s_cmd_count == 0 && !s_pace_pending) {
    return;
  }

  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return; // outbox busy/unavailable; retried on next callback or enqueue
  }

  if (s_cmd_count > 0) {
    dict_write_int32(iter, KEY_COMMAND, s_cmd_queue[s_cmd_head]);
    if (app_message_outbox_send() == APP_MSG_OK) {
      s_in_flight = OUTBOX_CMD; // popped on confirmed send
    }
  } else {
    dict_write_int32(iter, KEY_TARGET_PACE, s_pace_value);
    if (app_message_outbox_send() == APP_MSG_OK) {
      s_in_flight = OUTBOX_PACE;
      s_pace_pending = false; // consumed; a newer value will re-arm this
    }
  }
}

static void handle_plan_message(DictionaryIterator *iterator) {
  if (run_session_is_active()) {
    return;
  }

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

  // Movement state first: it may auto-resume the run (PAUSED -> ACTIVE) so a
  // pace update in the same message is accepted. Processed while active or
  // auto-paused (we still need movement to detect the resume).
  Tuple *moving_tuple = dict_find(iterator, KEY_MOVING);
  if (moving_tuple && run_session_is_active()) {
    run_session_set_moving(moving_tuple->value->int32 == 1);
    run_window_update();
  }

  Tuple *pace_tuple = dict_find(iterator, KEY_CURRENT_PACE);
  if (pace_tuple && run_state_get() == RUN_ACTIVE) {
    pace_engine_update(pace_tuple->value->int32);
    run_window_note_live_pace(); // clears "Acquiring GPS" + refreshes
  }

  // Target pace is watch-authoritative (Settings / pace picker / UP-DOWN); the
  // phone never sends it back, so there is intentionally no inbound TARGET_PACE
  // handler here. Re-add one only if a phone-driven target is ever desired.

  Tuple *distance_tuple = dict_find(iterator, KEY_CURRENT_DISTANCE);
  if (distance_tuple && run_state_get() == RUN_ACTIVE) {
    uint32_t distance = (uint32_t)distance_tuple->value->int32;
    if (distance >= s_last_distance) {
      run_state_set_distance(distance);
      s_last_distance = distance;
    } else if (s_last_distance - distance > 100) {
      run_state_set_distance(distance);
      s_last_distance = distance;
    }
    run_window_update();
  }

  Tuple *hr_tuple = dict_find(iterator, KEY_HEART_RATE);
  if (hr_tuple && run_state_get() == RUN_ACTIVE) {
    hr_monitor_set_phone_bpm((uint8_t)hr_tuple->value->int32);
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

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  if (s_in_flight == OUTBOX_CMD) {
    cmd_pop();
  }
  // Pace was already consumed at send time; nothing to do here.
  s_in_flight = OUTBOX_IDLE;
  s_attempts = 0;
  outbox_drain();
}

static void outbox_failed_handler(DictionaryIterator *iterator,
                                  AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage send failed: %d", reason);
  OutboxInFlight what = s_in_flight;
  s_in_flight = OUTBOX_IDLE;

  if (++s_attempts >= OUTBOX_MAX_ATTEMPTS) {
    // Give up on this item so we stop transmitting while disconnected.
    if (what == OUTBOX_CMD) {
      cmd_pop();
    }
    // Pace: leaving it consumed (pending false) drops it unless a newer value
    // re-armed s_pace_pending during the failed attempt.
    s_attempts = 0;
  } else if (what == OUTBOX_PACE) {
    s_pace_pending = true; // re-arm the latest pace for another try
  }
  outbox_drain();
}

void comm_init(void) {
  s_plan_callback = NULL;
  s_last_distance = 0;
  s_cmd_head = 0;
  s_cmd_count = 0;
  s_pace_pending = false;
  s_pace_value = 0;
  s_in_flight = OUTBOX_IDLE;
  s_attempts = 0;

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
  if (command == CMD_START) {
    s_last_distance = 0;
  }

  if (s_cmd_count == CMD_QUEUE_MAX) {
    if (s_in_flight == OUTBOX_CMD) {
      return; // head is in flight; can't safely drop it, so drop the new one
    }
    cmd_pop(); // make room by dropping the oldest queued command
  }

  uint8_t tail = (uint8_t)((s_cmd_head + s_cmd_count) % CMD_QUEUE_MAX);
  s_cmd_queue[tail] = command;
  s_cmd_count++;
  outbox_drain();
}

void comm_send_target_pace(int32_t pace) {
  // Coalesce: only the latest target pace matters.
  s_pace_value = pace;
  s_pace_pending = true;
  outbox_drain();
}

void comm_set_plan_received_callback(CommPlanReceivedCallback callback) {
  s_plan_callback = callback;
}
