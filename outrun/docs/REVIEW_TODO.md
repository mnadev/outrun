# Outrun — Adversarial Review: P2/P3 TODO

Remaining findings from the adversarial review, prioritized. Tackle P2 before
P3.

## Fixed since the review

- P0/P1: GPS-track coords, ghost-race ahead/behind + units, danger heartbeat
  (`ea83561`, `da732e7`, `7df6204`).
- HR-on-pause + rival jump-scare gating (`f6dfb52`).
- Round menu clipping (`menu_layer_set_center_focused`) and gating phone
  pace/distance/HR to active runs (`33e1c4e`).
- `AppSettings` versioned persist blob (magic + version, defaults on mismatch)
  and unit-aware target-pace stepping (steps in display units). Covered by
  `test/test_settings.c`.

## P2 — real UX / perf / robustness issues

1. **`CMD_*` can drop under button-mash.** `comm_send_command` (and
   `comm_send_target_pace`) call `app_message_outbox_begin` and bail silently
   if the outbox is busy, so rapid pause/resume or pace adjustments can
   desync watch and phone. Retry/queue or coalesce.

2. **Long-press affordance for slower pace is undiscoverable.** In the run
   window UP/DOWN now step in display units (km/mi), but there's no on-screen
   hint that DOWN / long-press makes the target slower. Surface a hint.

3. **`window_destroy()` inside each `window_unload()`.** `menu_window.c`,
   `plans_window.c`, `settings_window.c`, `run_window.c` destroy the window
   from its own unload handler. Verify this is safe with animated pops
   (potential UAF); the common Pebble pattern is to destroy after
   `window_stack_pop` from the caller, not from unload.

4. **wscript JS globs include `node_modules`/`__tests__`.** `wscript:51-53`
   globs `src/pkjs/**/*.js` + `*.json`. These don't ship (webpack entry is only
   `index.js`, verified: `build/pebble-js-app.js` has 0 `describe(`/`jest`
   hits), but the glob is messy and slows/confuses the build. Scope it to
   exclude `node_modules` and `__tests__`.

## P3 — polish / tech-debt

1. **Hand-numbered `KEY_*` duplicates `messageKeys`/`index.js` `Keys`.** `comm.c`
   and `index.js` both hardcode `0..15`. It works (verified identical) but
   drifts silently. Drive both from `package.json` `messageKeys` (the SDK
   generates `MESSAGE_KEY_*`).
2. **Received-but-ignored keys.** `IS_RUNNING` and `RIVAL_TIME` are sent but
   never read on the watch; remove or wire up.
3. **`hr_monitor` "debug" naming is really a phone-HR override.** `hr_monitor.c`
   `set_debug_bpm` / `s_debug_mode` actually inject phone-supplied HR; rename
   for clarity.
4. **Offline plan defaults omit "Intervals".** `plan.c:store_load_defaults`
   ships Easy/Tempo/Zone2 but not Intervals (distance-based segments), and
   distance-based segments can't progress offline (no GPS distance). Add or
   document the omission.
5. **Dead code.** `haptic_pulse_soft/rapid/danger` and the `haptic_pace_too_*` /
   `haptic_hr_too_*` wrappers (`haptic_feedback.c`) are unused;
   `analytics.js` and `ghost_racing.createMockGhosts` are unused in shipping
   paths; `themes_haptic_danger()` is now only used as a one-shot entry cue.
   Prune or repurpose.
6. **Lead erodes even within tolerance.** `pace_engine.c:55-68` decreases
   `distance_from_killer` for any non-zero delta, including small within-soft
   deltas; the bar slowly empties even when "on target". Consider a deadband.
