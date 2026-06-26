/**
 * mock_watch.c - Host-test WatchInterface implementation.
 */

#include "mock_watch.h"

#include <stddef.h>

static uint32_t s_now;
static uint8_t s_hr;
static bool s_hr_available;
static uint32_t s_play_count;
static uint32_t s_cancel_count;
static const HapticPattern *s_last_pattern;

static void mock_play(const HapticPattern *pattern) {
  s_play_count++;
  s_last_pattern = pattern;
}

static void mock_cancel(void) { s_cancel_count++; }

static uint32_t mock_now(void) { return s_now; }

static uint8_t mock_hr(void) { return s_hr; }

static bool mock_hr_available(void) { return s_hr_available; }

static const WatchInterface MOCK_WATCH = {
    .play = mock_play,
    .cancel = mock_cancel,
    .now_seconds = mock_now,
    .heart_rate = mock_hr,
    .heart_rate_available = mock_hr_available,
};

void mock_watch_install(void) { watch_set(&MOCK_WATCH); }

void mock_watch_reset(void) {
  s_now = 0;
  s_hr = 0;
  s_hr_available = false;
  s_play_count = 0;
  s_cancel_count = 0;
  s_last_pattern = NULL;
}

void mock_watch_advance(uint32_t seconds) { s_now += seconds; }

void mock_watch_set_hr(uint8_t bpm, bool available) {
  s_hr = bpm;
  s_hr_available = available;
}

uint32_t mock_watch_play_count(void) { return s_play_count; }

uint32_t mock_watch_cancel_count(void) { return s_cancel_count; }

const HapticPattern *mock_watch_last_pattern(void) { return s_last_pattern; }
