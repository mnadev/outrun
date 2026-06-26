/**
 * hr_stats.c - Portable heart-rate average accumulator.
 */

#include "hr_stats.h"

#include <stddef.h>

void hr_stats_reset(HrStats *stats) {
  if (!stats) {
    return;
  }
  stats->sum = 0;
  stats->count = 0;
}

void hr_stats_add(HrStats *stats, uint8_t bpm) {
  if (!stats || bpm == 0) {
    return;
  }
  stats->sum += bpm;
  stats->count++;
}

uint32_t hr_stats_average(const HrStats *stats) {
  if (!stats || stats->count == 0) {
    return 0;
  }
  return stats->sum / stats->count;
}
