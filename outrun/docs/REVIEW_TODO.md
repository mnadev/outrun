# Outrun — Adversarial Review: P2/P3 TODO

Remaining findings from the adversarial review, prioritized. The P0/P1 issues
and two cheap P2 wins (HR-on-pause, rival jump-scare gating) were fixed in
commits `ea83561`, `da732e7`, `7df6204`, `f6dfb52`. This list is intentionally
not implemented now (don't sprawl); tackle P2 before P3.

## P2 — real UX / perf / robustness issues

1. **Menu windows clip on round (chalk).** `menu_window.c`, `settings_window.c`,
   `plans_window.c` create `menu_layer_create(bounds)` with full bounds and no
   round insetting; the bottom row (e.g. "Settings") is clipped by the round
   bezel. The run window already uses `PBL_IF_ROUND_ELSE(30, 4)` side inset —
   menus need the equivalent (consider `menu_layer_set_center_focused` or
   per-cell insets). Confirmed by screenshot on chalk. Applies to all three
   menu windows.

2. **`AppSettings` persisted as a raw struct blob with no version/magic.**
   `settings.c:11-28` only sanity-checks the HR zone. Adding/reordering fields
   in `AppSettings` (`settings.h`) silently misreads old persists on upgrade.
   Add a magic + version prefix and validate on load; fall back to defaults on
   mismatch.

3. **Pace/distance engines mutate while IDLE/PAUSED.** `comm.c:78-98` calls
   `pace_engine_update` / `run_state_set_distance` on any inbound message
   regardless of run state, so stale GPS arrives and the run screen shows
   non-zero pace/distance before start or after pause. Gate on
   `run_session_is_active()` (or RUN_ACTIVE).

4. **`CMD_*` can drop under button-mash.** `comm_send_command` (and
   `comm_send_target_pace`) call `app_message_outbox_begin` and bail silently
   if the outbox is busy, so rapid pause/resume or pace adjustments can
   desync watch and phone. Retry/queue or coalesce.

5. **MI-mode target-pace stepping + long-press affordance.** In `run_window.c`
   up/down step target by 15 sec/km even when units are miles, and the
   long-press direction (slower) is undiscoverable. Step in display units and
   surface the long-press hint.

6. **`window_destroy()` inside each `window_unload()`.** `menu_window.c:85`,
   `plans_window.c:56`, `settings_window.c:149`, `run_window.c:569` destroy the
   window from its own unload handler. Verify this is safe with animated pops
   (potential UAF); the common Pebble pattern is to destroy after
   `window_stack_pop` from the caller, not from unload.

7. **wscript JS globs include `node_modules`/`__tests__`.** `wscript:51-53`
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
