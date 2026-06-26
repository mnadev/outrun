/**
 * comm.h - Phone-Watch communication via AppMessage
 */

#pragma once

#include "pace_engine.h"
#include "run_state.h"

typedef void (*CommPlanReceivedCallback)(void);

void comm_init(void);
void comm_deinit(void);
void comm_send_command(int command);
void comm_send_target_pace(int32_t pace);
void comm_set_plan_received_callback(CommPlanReceivedCallback callback);

#define CMD_START 1
#define CMD_STOP 2
#define CMD_PAUSE 3
#define CMD_RESUME 4
