/**
 * hr_stats.h - Portable heart-rate average accumulator (no SDK).
 *
 * Averaging is plain math, so it lives in the portable core rather than in a
 * platform HR driver. Feed it one sample per run tick; zero samples are
 * ignored so "no reading" never drags the average down.
 */

#pragma once

#include <stdint.h>

typedef struct {
  uint32_t sum;
  uint32_t count;
} HrStats;

/** Clear the accumulator. */
void hr_stats_reset(HrStats *stats);

/** Add one BPM sample. Samples of 0 (no reading) are ignored. */
void hr_stats_add(HrStats *stats, uint8_t bpm);

/** Average BPM so far, or 0 when there are no samples. */
uint32_t hr_stats_average(const HrStats *stats);
