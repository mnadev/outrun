# Outrun — Adversarial Review: TODO

Remaining findings from the adversarial review, prioritized.

## Fixed in the latest review round

- **P0: watch→phone AppMessage keys never matched.** The C app sends raw integer
  keys (`comm.c` `KEY_COMMAND = 3`, `KEY_TARGET_PACE = 1`), but the SDK assigns
  named keys at base 10000 (`build/appinfo.json`: `COMMAND = 10003`). PebbleKit
  JS surfaces an unmapped inbound key as the integer string (`e.payload["3"]`),
  so `watch_commands.js` reading `payload.COMMAND` was always `undefined` — every
  Start/Stop/Pause/Resume and target-pace message from the watch was dropped, so
  a paired phone never started GPS. Now reads the raw integer key (name accepted
  too); test covers the real on-device numeric payload.
- **P1: phone clobbered the watch's target pace.** `sendPaceUpdate` echoed the
  phone's `targetPace` back every GPS update and `comm.c` applied it. The watch
  owns target pace; the phone no longer sends it, and the now-dead inbound
  `TARGET_PACE` handler on the watch was removed.
- **P2: summary average pace was always labelled "/km".** Now suffixed `/mi` or
  `/km` per unit (`run_window.c`).
- **GPS distance/pace robustness** (`pace_calculator.js`): replaced the flat
  100 m jump cap (which dropped legitimate movement across a sparse GPS gap) with
  a speed-based reject (>12.5 m/s between fixes); added a 2 m stationary-jitter
  floor; and reject out-of-order/duplicate timestamps (no more div-by-zero / pace
  spikes). New jest coverage.
- **GPS Kalman smoothing** (`gps_kalman.js`): a constant-velocity Kalman filter
  now smooths fixes; distance accumulates between smoothed positions and pace
  comes from the velocity estimate. Smoother pace = fewer false band buzzes, and
  noisy-track distance error drops from naive's ~+43% to ~-11% with no bias on a
  clean track. Tuned (accelNoise 0.4) and tested (filter + integration suites).
- **Honest GPS state** (`run_window.c`): before a real fix arrives the watch no
  longer shows the target as if it were live pace; it shows `--:--` with
  "Acquiring GPS" -> "No GPS signal" (>30s) -> "GPS lost" (feed stale >10s).
  Verified on the emulator.
- **GPS auto-pause** (`auto_pause.c` + phone `MOVING` key): paired runs auto-
  pause after a 5s stop and resume on movement; the phone keeps GPS on while
  auto-paused so it can drive the resume. Watch stays run-state authoritative
  (manual pause/resume overrides; shows "Auto-paused" vs "Paused"). Pure
  decision module host-tested (9 Unity tests) + a pace_calculator speed test.
- **P2: phone-HR override was sticky** (`hr_monitor.c`): a once-received phone HR
  shadowed the on-watch sensor forever. Cleared on `hr_monitor_stop()`; renamed
  the misleading `debug` naming to phone-HR terms.
- **P2: inbound HR gating** now matches pace/distance (`RUN_ACTIVE` only).
- **P3: dead keys** `IS_RUNNING` / `RIVAL_TIME` are no longer sent (watch never
  read them); keys stay declared for index alignment.
- **P3: long-run time** now shows `H:MM:SS` past an hour (`run_window.c`).
- **P3: HR-zone stepping** preserves band width at the 60/220 clamps
  (`settings.c`, with Unity coverage).

## Fixed in earlier rounds

- GPS-track coords, ghost-race ahead/behind + units, danger heartbeat
  (`ea83561`, `da732e7`, `7df6204`).
- HR-on-pause + rival jump-scare gating (`f6dfb52`).
- Round menu clipping and gating phone pace/distance/HR to active runs
  (`33e1c4e`).
- `AppSettings` versioned persist blob and unit-aware target-pace stepping
  (`459ae21`).
- Outbound AppMessage reliability: command FIFO + coalesced pace + retry
  (`9abce76`). Killer-lead deadband (`a77d2c5`). Alarm-style pace picker.
- wscript globs exclude node_modules/__tests__ (`7f74e2d`); dead haptic/ghost
  code removed (`fda0ea6`, `986d8e7`).

## Remaining — intentionally deferred

1. **P3: hand-numbered `KEY_*` duplicates `messageKeys`/`Keys`/`MessageKeys`.**
   `comm.c`, `index.js`, and `watch_commands.js` each hardcode `0..15`. It now
   works in both directions and is documented, but three copies can still drift.
   The clean fix is to drive everything from the SDK's generated `MESSAGE_KEY_*`
   (named keys, base 10000) end to end. Deferred deliberately: it rewrites the
   working comm protocol on both sides and the round-trip can't be verified in
   the emulator (no phone), so it's higher-risk than its payoff right now. Do it
   as a dedicated change with on-device testing.
2. **P3: GPS jitter floor vs. very slow movement.** The 2 m smoothed-step floor
   can under-count genuine sub-0.4 m/s movement. Acceptable for a *running*
   pacer; revisit if walking support is wanted.

## Candidate further improvements (not yet done)

- **Auto-pause toggle in Settings.** Auto-pause is on by default (only active
  when paired and the phone reports a sustained stop). A Settings switch would
  let runners who prefer raw elapsed time turn it off. Needs a settings field
  (bump the persist version) + a settings-menu row.
- **Full-run GPX.** `getGpsTrack()` only returns the last `windowSize` smoothed
  points, so server-synced GPX is truncated. Out of scope (cloud) but worth
  fixing if/when cloud save matters; the offline ghost already uses the full
  track kept in `index.js`.
3. **P3: Offline plan defaults omit "Intervals".** Its distance-based segments
   can't progress without GPS distance, so it would stall offline; it still works
   when paired (the phone supplies it). Leave omitted from offline defaults.
4. **P3: unused JS not in the shipped bundle.** `analytics.js` (+ its test) and
   `ghost_racing.createMockGhosts` aren't in index.js's require graph, so webpack
   never bundles them. Harmless; prune only during a repo tidy.
