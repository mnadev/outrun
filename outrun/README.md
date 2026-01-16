# 🏃 Outrun - The Social Horror Pacer

> *"Run like your life depends on it... because in Outrun, it does."*

A gamified survival fitness app that transforms your runs into slasher-movie escapes. Keep pace with your target or the **KILLER** catches you.

## Features

### 🔪 Core Gameplay
- **Distance from Killer** - Fall behind pace and IT gets closer
- **Horror Haptics** - Feel the heartbeat intensify as danger approaches
- **Segment Survival** - Beat your rivals on Strava segments or "die trying"

### 📱 Phone Integration
- GPS-based pace tracking via PebbleKit JS
- Strava OAuth for segment hunting
- Real-time pace coaching to your wrist

### 👻 Premium Features
- **5 Horror Themes** - Classic Slasher, Paranormal, Zombie, Alien, Werewolf
- **Ghost Racing** - Race against your past runs
- **Fear Factor Analytics** - Track your survival rate and composure

### 🏆 Social
- Leagues and Teams with weekly challenges
- "Composure" scoring system
- Horror-themed ranks (Final Girl → Victim)

## Quick Start

```bash
# Build for all platforms
pebble build

# Install to emulator
pebble install --emulator basalt

# Run C tests
cd test && make test

# Run JS tests  
cd src/pkjs && npm test
```

## Controls

| Button | Action |
|--------|--------|
| **SELECT** | Start/Pause run |
| **SELECT (hold)** | Stop run |
| **UP** | Faster target pace (-15s/km) |
| **DOWN** | Slower target pace (+15s/km) |

## Architecture

```
src/
├── c/               # Pebble C code
│   ├── pace_engine  # Pace calculation + game logic
│   ├── haptic       # Horror vibration patterns
│   ├── run_state    # Run lifecycle state machine
│   ├── run_window   # Main UI
│   ├── comm         # Phone communication
│   ├── features     # Feature gating
│   └── stalker_themes # Premium horror themes
│
└── pkjs/            # PebbleKit JavaScript
    ├── index.js         # Main entry + GPS
    ├── pace_calculator  # Haversine + rolling window
    ├── strava_auth      # OAuth 2.0
    ├── strava_segments  # Segment detection
    ├── ghost_racing     # Race past runs
    ├── analytics        # Fear Factor stats
    └── mock_backend     # Simulated leagues
```

## Platforms

Built and tested for all Pebble platforms:
- ✅ Aplite (original Pebble)
- ✅ Basalt (Pebble Time)
- ✅ Chalk (Pebble Time Round)
- ✅ Diorite (Pebble 2)
- ✅ Emery (Pebble Time 2)
- ✅ Flint (future)

## Horror Themes

| Theme | Stalker | Escape | Caught |
|-------|---------|--------|--------|
| 🔪 Classic | THE KILLER | ESCAPED! | CAUGHT! |
| 👻 Paranormal | THE SPIRIT | BANISHED! | POSSESSED! |
| 🧟 Zombie | THE HORDE | SURVIVED! | INFECTED! |
| 👽 Alien | THE XENOMORPH | ESCAPED POD! | CAPTURED! |
| 🐺 Werewolf | THE BEAST | DAWN BREAKS! | MAULED! |

## License

MIT © 2026 Outrun
