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
