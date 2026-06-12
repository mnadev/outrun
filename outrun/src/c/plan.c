/**
 * plan.c - Running plan model, store, and progression engine
 */

#include "plan.h"
#include <string.h>

static RunPlan s_plans[MAX_PLANS];
static uint8_t s_plan_count;
static uint8_t s_expected_total;

static void add_default_plan(uint8_t index, const char *name,
                             const uint8_t *packed, size_t len) {
  uint8_t seg_count = (uint8_t)(len / SEGMENT_PACKED_SIZE);
  plan_store_add(index, name, seg_count, packed, len);
}

void plan_store_init(void) {
  s_plan_count = 0;
  s_expected_total = 0;
  memset(s_plans, 0, sizeof(s_plans));
}

bool plan_parse_segment(const uint8_t *data, PlanSegment *segment) {
  if (!data || !segment) {
    return false;
  }

  uint8_t flags = data[0];
  segment->label = (SegmentLabel)(flags & 0x03);
  segment->end_type = (SegmentEndType)((flags >> 2) & 0x01);
  segment->target_type = (SegmentTargetType)((flags >> 3) & 0x03);
  segment->end_value = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
  segment->target_lo = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
  segment->target_hi = (uint16_t)data[5] | ((uint16_t)data[6] << 8);
  return true;
}

bool plan_store_add(uint8_t index, const char *name, uint8_t seg_count,
                    const uint8_t *packed_data, size_t data_len) {
  if (index >= MAX_PLANS || seg_count > MAX_SEGMENTS || !name || !packed_data) {
    return false;
  }
  if (data_len < (size_t)seg_count * SEGMENT_PACKED_SIZE) {
    return false;
  }

  RunPlan *plan = &s_plans[index];
  strncpy(plan->name, name, PLAN_NAME_MAX - 1);
  plan->name[PLAN_NAME_MAX - 1] = '\0';
  plan->segment_count = seg_count;

  for (uint8_t i = 0; i < seg_count; i++) {
    plan_parse_segment(packed_data + (i * SEGMENT_PACKED_SIZE), &plan->segments[i]);
  }

  if (index >= s_plan_count) {
    s_plan_count = index + 1;
  }
  return true;
}

void plan_store_set_expected_total(uint8_t total) { s_expected_total = total; }

uint8_t plan_store_expected_total(void) { return s_expected_total; }

void plan_store_clear(void) {
  s_plan_count = 0;
  s_expected_total = 0;
  memset(s_plans, 0, sizeof(s_plans));
}

uint8_t plan_store_count(void) { return s_plan_count; }

const RunPlan *plan_store_get(uint8_t index) {
  if (index >= s_plan_count) {
    return NULL;
  }
  return &s_plans[index];
}

const RunPlan *plan_store_get_by_index(uint8_t list_index) {
  return plan_store_get(list_index);
}

const char *plan_segment_label_name(SegmentLabel label) {
  switch (label) {
  case SEG_LABEL_WARMUP:
    return "Warmup";
  case SEG_LABEL_RUN:
    return "Run";
  case SEG_LABEL_RECOVER:
    return "Recover";
  case SEG_LABEL_COOLDOWN:
    return "Cooldown";
  default:
    return "Segment";
  }
}

void plan_progress_init(PlanProgress *progress) {
  progress->active = false;
  progress->plan_index = 0;
  progress->segment_index = 0;
  progress->segment_elapsed_sec = 0;
  progress->segment_distance_m = 0;
  progress->completed = false;
}

bool plan_progress_start(PlanProgress *progress, uint8_t plan_index) {
  const RunPlan *plan = plan_store_get(plan_index);
  if (!plan || plan->segment_count == 0) {
    return false;
  }

  progress->active = true;
  progress->plan_index = plan_index;
  progress->segment_index = 0;
  progress->segment_elapsed_sec = 0;
  progress->segment_distance_m = 0;
  progress->completed = false;
  return true;
}

static bool segment_is_complete(const PlanSegment *seg, uint32_t elapsed,
                                uint32_t distance) {
  if (seg->end_type == SEG_END_DURATION) {
    return elapsed >= seg->end_value;
  }
  return distance >= seg->end_value;
}

bool plan_progress_advance(PlanProgress *progress) {
  const RunPlan *plan = plan_store_get(progress->plan_index);
  if (!plan) {
    return false;
  }

  progress->segment_index++;
  progress->segment_elapsed_sec = 0;
  progress->segment_distance_m = 0;

  if (progress->segment_index >= plan->segment_count) {
    progress->active = false;
    progress->completed = true;
    return true;
  }
  return true;
}

void plan_progress_tick(PlanProgress *progress, uint32_t elapsed_sec,
                        uint32_t distance_m) {
  if (!progress->active) {
    return;
  }

  const RunPlan *plan = plan_store_get(progress->plan_index);
  if (!plan) {
    return;
  }

  const PlanSegment *seg = &plan->segments[progress->segment_index];
  progress->segment_elapsed_sec = elapsed_sec;
  progress->segment_distance_m = distance_m;

  if (segment_is_complete(seg, elapsed_sec, distance_m)) {
    plan_progress_advance(progress);
  }
}

const PlanSegment *plan_progress_current_segment(const PlanProgress *progress) {
  if (!progress->active) {
    return NULL;
  }
  const RunPlan *plan = plan_store_get(progress->plan_index);
  if (!plan || progress->segment_index >= plan->segment_count) {
    return NULL;
  }
  return &plan->segments[progress->segment_index];
}

uint32_t plan_progress_remaining(const PlanProgress *progress) {
  const PlanSegment *seg = plan_progress_current_segment(progress);
  if (!seg) {
    return 0;
  }

  if (seg->end_type == SEG_END_DURATION) {
    if (progress->segment_elapsed_sec >= seg->end_value) {
      return 0;
    }
    return seg->end_value - progress->segment_elapsed_sec;
  }

  if (progress->segment_distance_m >= seg->end_value) {
    return 0;
  }
  return seg->end_value - progress->segment_distance_m;
}

void plan_store_load_defaults(void) {
  if (s_plan_count > 0) {
    return;
  }

  // Easy Run: 20 min pace band 5:00-5:30/km
  static const uint8_t easy_run[] = {
      0x09, 0xB0, 0x04, 0x2C, 0x01, 0x4A, 0x01,
  };
  add_default_plan(0, "Easy Run", easy_run, sizeof(easy_run));

  // Tempo: 5min warmup, 15min tempo 4:45-5:00, 5min cooldown
  static const uint8_t tempo[] = {
      0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x09, 0x84, 0x03, 0x1D, 0x01, 0x2C, 0x01,
      0x03, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00,
  };
  add_default_plan(1, "Tempo", tempo, sizeof(tempo));

  // Zone 2 Base: 30 min HR band 120-150
  static const uint8_t zone2[] = {
      0x11, 0x08, 0x07, 0x78, 0x00, 0x96, 0x00,
  };
  add_default_plan(2, "Zone 2 Base", zone2, sizeof(zone2));
}
