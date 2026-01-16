/**
 * comm.h - Phone-Watch communication via AppMessage
 */

#pragma once

#include "pace_engine.h"
#include "run_state.h"
#include <pebble.h>

/**
 * Initialize AppMessage communication.
 */
void comm_init(void);

/**
 * Deinitialize AppMessage communication.
 */
void comm_deinit(void);

/**
 * Send a command to the phone (start/stop/pause/resume).
 */
void comm_send_command(int command);

/**
 * Send target pace update to phone.
 */
void comm_send_target_pace(int32_t pace);

// Commands
#define CMD_START 1
#define CMD_STOP 2
#define CMD_PAUSE 3
#define CMD_RESUME 4
