# Outrun — Adversarial Review: TODO

Remaining findings from the adversarial review, prioritized. Tackle P2 before
P3.

## Fixed in the latest review round

- **P0: watch→phone AppMessage keys never matched.** The C app sends raw integer
  keys (`comm.c` `KEY_COMMAND = 3`, `KEY_TARGET_PACE = 1`), but the SDK assigns
  named keys at base 10000 (`build/appinfo.json`: `COMMAND = 10003`). PebbleKit
  JS surfaces unmapped inbound keys as the integer string (`e.payload["3"]`), so
  `watch_commands.js` reading `payload.COMMAND` was always `undefined` — every
  Start/Stop/Pause/Resume and target-pace message from the watch was dropped, so
  a paired phone never started GPS. Now read the raw integer key (name accepted
  too); test flipped to cover the real on-device numeric payload
  (`watch_commands.js`, `__tests__/watch_commands.test.js`).
- **P1: phone clobbered the watch's target pace.** `sendPaceUpdate` echoed the
  phone's `targetPace` (default 5:00) back every GPS update, and `comm.c` applied
  it via `pace_engine_set_target`, overwriting the runner's chosen pace once the
  paired path worked. The watch owns target pace; the phone no longer sends it
  (`index.js`).
- **P2: summary average pace was always labelled "/km".** `settings_format_pace`
  already converts to the display unit, so miles mode showed a per-mile number
  under a "/km" label. Now suffixed `/mi` or `/km` per unit (`run_window.c`).

## Fixed in earlier rounds

- GPS-track coords, ghost-race ahead/behind + units, danger heartbeat
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
- Alarm-style MM:SS target-pace picker (`pace_window.c`).

## P2 — remaining, verify on hardware

1. **Phone-HR override is sticky.** `hr_monitor.c` `set_debug_bpm` sets
   `s_debug_mode = true` permanently and `get_bpm` then returns the last
   phone-supplied value forever, shadowing the real on-watch sensor across later
   runs (until app relaunch). Only affects mixed paired/unpaired HR use; the
   offline-only on-watch HR path is unaffected. Recommended fix: clear the
   override on run start (e.g. in `hr_monitor_start`) or expire it by timestamp.
   Deferred because it isn't exercisable in the emulator (no phone, no HR sensor)
   and touches the HR lifecycle on a path the core offline product doesn't use.
2. **Inbound HR gating differs from pace/distance.** `comm.c` accepts phone HR
   while `RUN_ACTIVE || RUN_PAUSED` but pace/distance only while `RUN_ACTIVE`.
   Harmless today (the phone stops tracking on pause), but inconsistent.

## P3 — polish / tech-debt

1. **Hand-numbered `KEY_*` duplicates `messageKeys`/`index.js` `Keys`.** `comm.c`,
   `index.js`, and now `watch_commands.js` all hardcode `0..15`. It works (and is
   now correct in both directions), but drifts silently. Either drive both sides
   from the SDK's generated `MESSAGE_KEY_*` (named keys, base 10000) end-to-end,
   or add a shared generated constant; document the choice.
2. **Received-but-ignored / sent-but-unused keys.** `IS_RUNNING` and `RIVAL_TIME`
   are sent but never read on the watch; with the P1 fix, inbound `TARGET_PACE`
   on the watch is now read-but-never-sent. Remove or wire up.
3. **`hr_monitor` "debug" naming is really a phone-HR override.** `set_debug_bpm`
   / `s_debug_mode` inject phone-supplied HR; rename for clarity.
4. **Offline plan defaults omit "Intervals".** `plan.c:store_load_defaults`
   ships Easy/Tempo/Zone2 but not Intervals. Intervals uses distance-based
   segments, which can't progress without GPS distance, so adding it to the
   offline defaults would stall; leave omitted unless distance comes from an
   on-watch source. (Intervals still works when paired — the phone supplies it.)
5. **Elapsed time has no HH:MM:SS.** `format_elapsed` prints `MMM:SS`, so runs
   over 99 min read e.g. `150:23`. Fine for typical runs; tidy later.
6. **HR-zone stepping saturates unevenly.** `settings_window.c` adds/subtracts 5
   to both bounds; near the 60/220 clamps the band width shrinks. Cosmetic.
7. **Dead JS not in the bundle.** `analytics.js` (+ its test) and
   `ghost_racing.createMockGhosts` are unused in shipping paths but harmless
   (webpack only bundles index.js's graph). Prune if doing a repo tidy.
