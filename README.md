# mazelock

Linux fullscreen maze game / screen locker. Navigate a 3D ray-cast maze with arrow keys. Password is currently hardcoded [password\_overlay.hpp](https://github.com/10per5/mazelock/blob/main/src/security/password_overlay.hpp#L11), or with premake5 param.

## Build

Use `predep` or `premake5 gmake2` to build the project.

## Controls

| Key                       | Action                             |
| ------------------------- | ---------------------------------- |
| Arrow keys                | Move / turn                        |
| Tab                       | Sprint (3x speed)                  |
| Escape                    | Toggle lock screen                 |
| F11 / Arrow Keys          | Toggle auto-walk (right-hand rule) |
| F12 (`--debug` mode only) | Quit                               |

## Dependencies

* X11, Xinerama (primary backend)

* libdrm (fallback)

* Linux fbdev (last resort)

* libpng (optional, `--usepng`)

## License

GPL v3
