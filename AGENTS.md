# Build System

- `premake5 gmake2` regenerates makefiles; build with `predep`
- Edit `premake5.lua` only, not generated makefiles
- New `.cpp` files are auto-detected via `files({ "src/**.cpp" })`
- C++23 (`cppdialect("C++23")`)

# Architecture

## World (`world.hpp/cpp`) — game loop owner
- All owned via `std::unique_ptr<T>`; no hand-written destructor (RAII)
- `run()`: event loop with input → state update → render
- Global `Config cfg` holds key-value settings from config file + CLI args
- Global `Logger g_logger` — debug output; `g_logger.set_enabled(cfg.debug_mode())` at world init. `g_logger.debug(fmt, args...)` is a no-op when disabled (no `std::format` overhead)

## Player / Walker (`player/`, `player/walker.hpp`)
- **Walker** — interpolation engine. No maze knowledge. Tracks `cell_x/y`, `direction_`, `step_` (0→1), `turning_`, `steps_`. Methods: `plan_move/turn`, `teleport`, `hold_position`, `update(...)`, `manual_forward/back/turn_left/right`
- **Player** — owns `AutoWalkStrategy` + `GoalSeekerStrategy` via `std::unique_ptr`; delegates to `WalkStrategy* current_`
- **AutoWalkStrategy** — right-hand rule + path-following + animal-reverse logic. Owns one `Walker`
- **GoalSeekerStrategy** — follows a precomputed pathfind path

## Entity hierarchy (`entity/`)
- `Entity` — abstract base with virtual dispatch (no `dynamic_cast` at call sites):
  - `update(float dt)` — default no-op, `Animal` overrides
  - `try_consume_animal(maze, px, py, x, y)` — `Animal` overrides → starts flee
  - `try_consume_coin(maze, x, y)` — `Coin` overrides → marks inactive
  - `render(...)` — pure virtual, all leaf classes implement
  - `occupies(x,y)`, `minimap_color()`, `world_x/y()` — pure virtual
- Leaf classes marked `final`: `Animal`, `Coin`, `Goal`

## Render
- `Framebuffer` — DRM / X11 / fbdev backend; `std::span<uint8_t> screen()`
- `Raycaster` — raycasts into `color_buffer_` / `depth_buffer_` (both `std::vector<uint32_t>`)
- `RenderManager` — orchestrates raycaster → sprites → overlay → drawer

## Debug output
- `Logger` abstract base (`cfg/logger.hpp`) — two subclasses inline:
  - **NullLogger** — `is_active()` returns false, no formatting, zero runtime cost
  - **DebugLogger** — `is_active()` returns true, formats + prints
- `Logger* g_logger` declared in `cfg/singletons.hpp`, defined as `nullptr` in `singletons.cpp`; set in `World::World()` based on `--debug` flag
- All debug output uses `g_logger->debug(fmt, args...)`; no raw `printf`/`std::println` + `if` guards

# Key Conventions

- Smart pointers (`std::unique_ptr`) everywhere; no raw `new`/`delete`
- `#pragma once` for headers
- `override` / `final` on virtual methods
- `std::print` / `std::println` for error output (via `g_logger` or `std::println(stderr, ...)`)
- `std::from_chars` for string→number conversion
- No `dynamic_cast` in dispatch paths — use virtual methods
