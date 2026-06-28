# Outrun — Horror Pacer for Pebble

A Pebble running pacer: your phone tracks GPS pace and distance, your watch
shows targets and buzzes when you drift off pace or heart rate.

## What it does

- **Phone → watch pace & distance** — PebbleKit JS uses GPS to compute current
  pace and total distance (smoothed with a constant-velocity Kalman filter), sent
  to the watch over Bluetooth. The watch shows "Acquiring GPS" until a real fix
  arrives instead of faking it, and **auto-pauses** when you stop.
- **On-watch HR** — Pebble Health (Pebble 2 / Time 2) samples heart rate during
  a run; zone shown on screen.
- **Vibration alerts** — Debounced buzzes when pace or HR leaves your target
  band (configurable in Settings).
- **Horror skin** — "Distance from killer" bar, theme voice, chase heartbeat
  when you fall behind. Optional ghost rival races your last run (offline).
- **Structured plans** — Preset workouts (easy, tempo, zone 2) with segment
  targets; works offline with built-in defaults.

Works **standalone on the watch** without a server. Strava and cloud save are
optional extras that degrade gracefully when unavailable.

## Quick start

```bash
pebble build
pebble install --emulator basalt

cd test && make          # C unit tests
cd src/pkjs && npm test  # JS unit tests
```

Pair with the Pebble app on your phone. Start a **Quick Run** on the watch;
the phone companion starts GPS when it receives the start command.

## Controls

| Button | Action |
|--------|--------|
| **SELECT** | Start / pause / resume |
| **SELECT (hold)** or **BACK** | Stop run (summary) |
| **UP / DOWN** | Faster / slower target pace (quick run only) |

Settings: units (km/mi), target pace, HR zone, alert toggles, stalker theme.

## Platforms

aplite, basalt, chalk, diorite, emery, flint — all build with `-Werror`.

## Architecture

Portable C core (`pace_engine`, `alert_engine`, `run_state`, `plan`) behind
`WatchInterface`; Pebble adapter in `pebble_watch.c`. Phone logic in
`src/pkjs/` (`pace_calculator.js`, `index.js`). See `docs/PORTING.md`.

## License

MIT © 2026 Outrun
