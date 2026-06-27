# Outrun — Adversarial Review: P2/P3 TODO

Remaining findings from the adversarial review, prioritized. Tackle P2 before
P3.

## Fixed since the review

- P0/P1: GPS-track coords, ghost-race ahead/behind + units, danger heartbeat
  (`ea83561`, `da732e7`, `7df6204`).
- HR-on-pause + rival jump-scare gating (`f6dfb52`).
- Round menu clipping (`menu_layer_set_center_focused`) and gating phone
  pace/distance/HR to active runs (`33e1c4e`).
- `AppSettings` versioned persist blob and unit-aware target-pace stepping
  (`459ae21`, `test/test_settings.c`).
- Outbound AppMessage reliability: command FIFO + coalesced pace + retry,
  instead of silent drop (`9abce76`).
- Killer-lead deadband: hold steady while on target (`a77d2c5`).
- wscript globs scoped to exclude node_modules/__tests__ (`7f74e2d`).
- Dead code: removed unused haptic wrappers (`fda0ea6`) and unused
  ghost_racing requires that bloated the bundle ~7KB (`986d8e7`).
- `window_destroy()`-in-`unload()`: stress-tested rapid push/pop of all
  windows on basalt; stable, no crash. Left as-is (rewriting the lifecycle
  across 4 windows would add risk for no observed benefit).
- Target-pace UX: replaced the undiscoverable click/long-click stepping with a
  full-screen alarm-style MM:SS picker (`pace_window.c`), opened from Settings.
  Removed the now-dead `settings_adjust_target_pace`.

## P3 — remaining polish / tech-debt

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
   ships Easy/Tempo/Zone2 but not Intervals. Intervals uses distance-based
   segments, which can't progress offline (no GPS distance), so adding it to
   the offline defaults would stall; leave omitted unless distance comes from
   an on-watch source.
5. **Dead JS not in the bundle.** `analytics.js` (+ its test) and
   `ghost_racing.createMockGhosts` are unused in shipping paths but harmless
   (webpack only bundles index.js's graph). Prune if doing a repo tidy.
