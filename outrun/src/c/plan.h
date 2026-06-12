/**
 * plan.h - Running plan model, store, and progression engine
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_PLANS 8
#define MAX_SEGMENTS 16
#define SEGMENT_PACKED_SIZE 7
#define PLAN_NAME_MAX 24

typedef enum {
  SEG_LABEL_WARMUP = 0,
  SEG_LABEL_RUN = 1,
  SEG_LABEL_RECOVER = 2,
  SEG_LABEL_COOLDOWN = 3
} SegmentLabel;

typedef enum {
  SEG_END_DURATION = 0,
  SEG_END_DISTANCE = 1
} SegmentEndType;

typedef enum {
  SEG_TARGET_NONE = 0,
  SEG_TARGET_PACE = 1,
  SEG_TARGET_HR = 2
} SegmentTargetType;

typedef struct {
  SegmentLabel label;
  SegmentEndType end_type;
  uint16_t end_value;
  SegmentTargetType target_type;
  uint16_t target_lo;
  uint16_t target_hi;
} PlanSegment;

typedef struct {
  char name[PLAN_NAME_MAX];
  uint8_t segment_count;
  PlanSegment segments[MAX_SEGMENTS];
} RunPlan;

typedef struct {
  bool active;
  uint8_t plan_index;
  uint8_t segment_index;
  uint32_t segment_elapsed_sec;
  uint32_t segment_distance_m;
  bool completed;
} PlanProgress;

void plan_store_init(void);
bool plan_store_add(uint8_t index, const char *name, uint8_t seg_count,
                    const uint8_t *packed_data, size_t data_len);
void plan_store_clear(void);
void plan_store_set_expected_total(uint8_t total);
uint8_t plan_store_expected_total(void);
uint8_t plan_store_count(void);
const RunPlan *plan_store_get(uint8_t index);
const RunPlan *plan_store_get_by_index(uint8_t list_index);

bool plan_parse_segment(const uint8_t *data, PlanSegment *segment);
const char *plan_segment_label_name(SegmentLabel label);

void plan_progress_init(PlanProgress *progress);
bool plan_progress_start(PlanProgress *progress, uint8_t plan_index);
void plan_progress_tick(PlanProgress *progress, uint32_t elapsed_sec,
                        uint32_t distance_m);
bool plan_progress_advance(PlanProgress *progress);
const PlanSegment *plan_progress_current_segment(const PlanProgress *progress);
uint32_t plan_progress_remaining(const PlanProgress *progress);

/** Load compiled-in default plans when phone is unreachable. */
void plan_store_load_defaults(void);
