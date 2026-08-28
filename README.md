# Wall Arena

A top-down local PvP tank arena game built in C++ with SDL2. Designed for projection mapping onto building walls using an ILDA laser projector.

## Features

- **Fast-paced arena combat** — Inspired by Unreal Tournament / Quake. Quick respawns, power-up control.
- **Variable player count** — Players join/leave mid-match via keyboard or gamepad.
- **Three game modes**:
  - **Deathmatch** — First to N kills wins
  - **Time Limit** — Most kills when time expires wins
  - **Last Man Standing** — Limited respawns, last player alive wins
- **Power-ups** — Auto-aim, homing rockets, shield, rapid fire
- **Map editor** — Place obstacles, spawn points, power-up locations. Tag obstacles for wall-feature alignment.
- **Dual render output** — SDL2 window + ILDA laser projector file output
- **Vector-only rendering** — All graphics are lines/polylines, native to laser projectors

## Building

Requires CMake 3.16+ and a C++17 compiler. SDL2 is fetched automatically.

```bash
cmake -S . -B build
cmake --build build --config Release
```

The executable will be at `build/Release/WallArena.exe` (Windows) or `build/WallArena` (Linux/macOS).

## Controls

### Menu
| Key | Action |
|-----|--------|
| Up/Down | Navigate menu |
| Left/Right | Cycle game mode |
| Enter | Select |
| Escape | Back / Quit |

### Gameplay
| Player 1 | Player 2 | Action |
|-----------|-----------|--------|
| W/S | Up/Down | Move forward/back |
| A/D | Left/Right | Turn |
| Space | Right Ctrl | Fire |

Gamepads: Left stick to move/turn, A / RB / RT to fire.

### Map Editor
| Key | Action |
|-----|--------|
| 1 | Select tool |
| 2 | Draw obstacle (left-click vertices, right-click to finish) |
| 3 | Place spawn point |
| 4 | Place power-up (Tab to cycle type) |
| 5 | Delete tool |
| G | Toggle grid snap |
| Z | Undo |
| S | Save map |
| Escape | Back to menu |

## ILDA Output

The `ILDARenderer` writes ILDA Format 4 (2D + true color) `.ild` files. To enable dual output, uncomment the line in `Game.cpp`:

```cpp
m_renderers.push_back(m_ildaRenderer.get());
```

The output file is written to `output.ild` on shutdown.

## Network Output (live hardware backends)

`NetRenderer` is enabled by default. It opens a TCP server (port `9600`, override
with the `WALLARENA_NET_PORT` env var, `0` to disable) and broadcasts every
rendered frame as vector geometry to all connected clients. This lets an external
process drive real hardware — an oscilloscope in X-Y mode, a laser galvo DAC, a
custom visualiser — without linking against the game.

The broadcast rate is capped (default 60 Hz, `WALLARENA_NET_FPS`). Frames are
only serialised while a client is connected, and a client that can't keep up is
dropped rather than stalling the game loop. POSIX sockets only; on Windows the
renderer compiles to a no-op.

**Wire format** — one `\n`-terminated text line per frame, space-separated
fields, paths separated by ` ; `:

```
F <worldW> <worldH> <numPaths> ; <r> <g> <b> <n> <x0> <y0> ... ; ...
```

Coordinates are world units (same space as the `IRenderer` draw calls, Y down).
Each path is a pen-down polyline; the client blanks the beam between paths.

## Project Structure

```
src/
├── main.cpp              — Entry point
├── core/
│   ├── Game.h/.cpp       — Game loop, state machine
│   └── Vec2.h            — 2D vector math
├── game/
│   ├── Tank.h/.cpp       — Tank entity
│   ├── Projectile.h/.cpp — Bullet / homing rocket
│   ├── PowerUp.h/.cpp    — Power-up pickups & effects
│   ├── Arena.h/.cpp      — Gameplay coordinator
│   └── Collision.h/.cpp  — Segment & polygon collision
├── input/
│   ├── InputManager.h/.cpp — Keyboard & gamepad input
│   └── PlayerInput.h      — Per-player input state
├── render/
│   ├── IRenderer.h        — Renderer interface
│   ├── SDLRenderer.h/.cpp — SDL2 window backend
│   ├── ILDARenderer.h/.cpp — ILDA file output backend
│   ├── NetRenderer.h/.cpp — TCP frame broadcast backend
│   └── Color.h            — RGBA color
├── map/
│   ├── Map.h/.cpp         — Map data structure
│   ├── Obstacle.h/.cpp    — Polygon obstacle
│   └── MapSerializer.h/.cpp — Text-based save/load
└── editor/
    └── MapEditor.h/.cpp   — Interactive map editor
```

## License

MIT
