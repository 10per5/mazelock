#pragma once

#include <termios.h>

class TerminalGuard {
    termios orig_{};

public:
    TerminalGuard();
    ~TerminalGuard();

    TerminalGuard(const TerminalGuard&) = delete;
    TerminalGuard& operator=(const TerminalGuard&) = delete;
};
