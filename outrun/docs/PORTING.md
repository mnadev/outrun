# Porting Outrun to other watch platforms

Outrun's running logic is deliberately split so the "brain" never touches a
watch SDK. Porting to a new platform (Garmin Connect IQ, Wear OS, …) means
writing one small **adapter** and reusing everything else.

## Architecture

```
┌─────────────────────────────────────────────┐
│ Portable core  (pure C, no <pebble.h>)        │
│   alert_engine     pace/HR band -> AlertType   │
│   plan             structured workout segments │
│   run_state        run lifecycle + timing      │
│   haptic_patterns  HapticEvent -> HapticPattern │
│   feedback         event -> pattern -> watch()  │
│   watch.c          holds the active interface   │
└───────────────┬───────────────────────────────┘
                │  WatchInterface (the only seam)
        ┌───────┴────────┬─────────────────┐
        ▼                ▼                 ▼
   pebble_watch     garmin (Monkey C)   wear os (Kotlin)
   vibes_* / time   Attention.vibrate   VibrationEffect
```

The seam is a single vtable in `src/c/watch_interface.h`:

```c
typedef struct {
  // Output
  void (*play)(const HapticPattern *pattern);  // fire one vibration
  void (*cancel)(void);
  // Input
  uint32_t (*now_seconds)(void);
  uint8_t  (*heart_rate)(void);
  bool     (*heart_rate_available)(void);
} WatchInterface;
```

A `HapticPattern` is just data: `durations` is an array of millisecond
segments that **alternate vibe-on / pause, starting with vibe-on**, and
`count` is its length.

## What is shared vs. reimplemented

| Layer | Pebble | Garmin / Wear OS |
| --- | --- | --- |
| Alert/zone/plan/timing logic | `*.c` (C) | reimplement in Monkey C / Kotlin, **using the C as the spec** |
| Haptic pattern *values* | `haptic_patterns.c` | copy the same millisecond arrays |
| `WatchInterface` *contract* | `pebble_watch.c` | implement the same 5 methods |
| Host unit tests | `test/` | the authoritative behavior spec to port against |

C source does not run on Garmin (Monkey C) and is not used on Wear OS (Kotlin),
so the port is a faithful reimplementation. The value of this architecture is
that the algorithms are isolated, named, and locked down by tests — so the
reimplementation is mechanical rather than archaeological.

Keep the pattern table identical across platforms (consider generating each
language's table from one shared data file to prevent drift).

## Adding a platform

1. Implement the 5 `WatchInterface` methods natively.
2. Reimplement the pattern table (`HapticEvent -> durations[]`) with the same
   values as `haptic_patterns.c`.
3. Reimplement the pure modules (`alert_engine`, `plan`, `run_state`) against
   the host tests in `test/`.
4. Drive the run loop: each tick, build the band input, get an `AlertType`,
   map it to a `HapticEvent`, and call `play(pattern)`.

## Garmin Connect IQ (Monkey C)

Garmin vibration uses `Toybox.Attention.vibrate`, which takes an array of
`VibeProfile(dutyCycle, length)` — `dutyCycle` is intensity 0–100, `length`
is milliseconds. Map each vibe-on segment to a profile; skip the pause
segments (or model them as `VibeProfile(0, pause)`).

```monkeyc
using Toybox.Attention;

class GarminWatch {
    // durations alternate on/off, starting ON (same as the C core)
    function play(durations) {
        if (!(Attention has :vibrate)) { return; }
        var profiles = [];
        for (var i = 0; i < durations.size(); i++) {
            var on = (i % 2 == 0);
            profiles.add(new Attention.VibeProfile(on ? 100 : 0, durations[i]));
        }
        Attention.vibrate(profiles);
    }

    function nowSeconds() { return Time.now().value(); }
    function heartRate() {
        var info = Activity.getActivityInfo();
        return (info != null && info.currentHeartRate != null)
            ? info.currentHeartRate : 0;
    }
}
```

Connect IQ is also where paid apps live, so this is the commercial target.

## Wear OS (Kotlin / Android)

Android uses `VibrationEffect.createWaveform(timings, repeat)`. **Gotcha:**
Android waveform timings start with an **off** segment, but our core arrays
start **on** — so prepend a `0`.

```kotlin
class WearWatch(context: Context) : WatchInterface {
    private val vibrator =
        context.getSystemService(Context.VIBRATOR_SERVICE) as Vibrator

    override fun play(durations: IntArray) {
        // core starts ON; Android waveform starts OFF -> prepend 0
        val timings = LongArray(durations.size + 1)
        for (i in durations.indices) timings[i + 1] = durations[i].toLong()
        vibrator.vibrate(VibrationEffect.createWaveform(timings, -1))
    }

    override fun cancel() = vibrator.cancel()
    override fun nowSeconds(): Long = System.currentTimeMillis() / 1000
    // heartRate(): read from HealthServices / SensorManager
}
```

## Reference: Pebble adapter

See `src/c/pebble_watch.c` for the canonical, minimal implementation
(`vibes_enqueue_custom_pattern`, `vibes_cancel`, `time()`, and `hr_monitor`).
