# Build System

- **Project generator:** premake5 (run `premake5 gmake2` to regenerate makefiles)
- **Build command:** `predep` (not make/cmake directly)
- **Config:** `premake5.lua` — update this file, not generated makefiles
- **Adding new .cpp files:** premake5.lua uses `files({ "src/**.cpp" })`, so new files under `src/` are auto-detected after regeneration

# Architecture — Walker/AI Separation

The walker module (`src/algorithm/walker.hpp/cpp`) and the autoplay AI module (`src/algorithm/autoplay.hpp/cpp`) are now separated:

**Walker** — raw grid-step interpolation engine:
- `plan_move(dir)` / `plan_turn(dir)` / `teleport(x,y,dir)` / `hold_position()` — set up step targets
- `update(pos_x, pos_y, dir_x, dir_y, speed, on_complete_callback)` — advances interpolation and fires callback on each completed step. Callback returns `bool` (true = early bail → sets position to cell centre and returns).
- `restart_step()` — resets interpolation to t=0 (used by manual control)
- `hold_position()` — sets `turn_divider_ = 1e6f` to practically freeze advancement (used when manual mode is idle)
- Tracks: `cell_x/y`, `direction_`, `step_` (0→1), `turning_` flag, `finished_` flag, `steps_` counter.
- No maze knowledge; no decision-making.

**AutoplayAI** — decision layer built on Walker:
- Owns one `Walker` instance; wraps `walker_.update()` with an `on_step_complete` callback.
- Callback handles: `reverse_requested_`, consume‑check, `plan_next_step()` (path‑following / right‑hand rule / dead‑end pause).
- `plan_next_step()` uses `walker_.turning()` to force a forward move after any turn completes (prevents right‑hand rule from re‑evaluating mid‑corridor).
- Manual control methods (`manual_forward/back/turn_left/right`) delegate to `walker_.plan_* + walker_.restart_step()`.
- Manual mode (`manual_mode_` flag): `plan_next_step` calls `walker_.hold_position()` so the walker sits idle between key presses.

# Recent Changes

## Auto-walk consume-pause fix
- Added `consuming()` accessor to `Walker`; guarded `plan_next_step()` with `!walker_.consuming()` so the auto-walker does not inject moves during the consume-pause window.

## Animal flee distance (pathfinding)
- `start_flee()` now pathfinds 3–5 tiles away from the player (greedy farthest‑step per iteration). When the path is ≤2 cells, falls back to a 0.6s bounce‑in‑place + zoom‑out animation.

## Walk-in detection — manual mode
- `manual_forward()` / `flush_pending()` / `plan_move()` all check for an animal at the target cell before moving. On hit: the animal flees, player stays in place (no back-step), and auto-walk emits an `on_animal_` callback.
- Animal check was added directly in `plan_move()` to cover the case where `stepping_` is false.

## Auto-walk reverse on animal
- `reverse_steps_` counter (5–6 steps init) drives the full reverse sequence: ① bump‑in‑place (10 frames), ② consume‑pause (30 frames, flee animation plays), ③ 180° turn (two 90° turns), ④ walk forward 4–5 steps.
- `reversing_` flag tracks whether the 180° turn has been initiated; cleared when `reverse_steps_` reaches 0 or a wall blocks the reverse.
- After reverse finishes, right‑hand rule resumes (direction is now opposite from original).
- Consume‑pause no longer auto-injects a backward move — the strategy now decides the next action via `plan_next_step`.

## Bump animation
- `start_bump(int dir)` sets a 10‑frame counter (no‑op if already bumping); `update()` adds a brief forward‑then‑back position offset (sine curve) in the bump direction.
- The bump code now runs unconditionally at the end of every `update()` call (via `goto do_bump`), so it is not skipped by early returns (consume‑pause, finish, etc.).
- Triggered by `plan_move()`, `flush_pending()`, `manual_forward()`, and `manual_back()` when blocked by a wall or when walking into an animal.
- Allows the player to visually "attempt" the move while staying in the same cell — used in both manual and auto‑walk modes.

## Input binding
- Keys: `shift` renamed to `tab` across the input system.

## Sprite depth sorting
- Added `world_x()/world_y()` virtual to `Entity` base; implemented in `Coin`, `Goal`, `Animal`.
- `render_sprites()` now sorts entities by squared distance (farthest first) to ensure correct sprite‑sprite occlusion regardless of creation order.
