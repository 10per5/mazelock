# mazelock

Linux fullscreen maze game / screen locker. Navigate a 3D ray-cast maze with arrow keys. Press a key to lock — the maze darkens, a password overlay appears, and a pipe screensaver runs on secondary monitors. Unlock with hardcoded password (for now).

## Build

Use `predep` or `premake5 gmake2` to build the project. It's quite straightforward.

## Controls

| Key                     | Action                             |
| ----------------------- | ---------------------------------- |
| Arrow keys              | Move / turn                        |
| Shift                   | Sprint (3x speed)                  |
| A–Z, 0–9                | Lock screen (during play)          |
| Type `world90s` + Enter | Unlock                             |
| Escape                  | Cancel lock                        |
| F11                     | Toggle auto-walk (right-hand rule) |
| F11 x3                  | Pathfind to finish (BFS)           |
| Ctrl+Q (debug)          | Quit                               |

## Dependencies

- X11, Xinerama (primary backend)
- libdrm (fallback)
- Linux fbdev (last resort)
- libpng (optional, `--usepng`)

## License

GPL v3
