#include "terminal.hpp"

#include <unistd.h>

TerminalGuard::TerminalGuard() {
    ::tcgetattr(STDIN_FILENO, &orig_);
    termios raw = orig_;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

TerminalGuard::~TerminalGuard() {
    ::tcsetattr(STDIN_FILENO, TCSANOW, &orig_);
}
