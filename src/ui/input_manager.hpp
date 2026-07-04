#pragma once

#include <chrono>
#include <vector>

#include <X11/Xlib.h>
#include <X11/keysym.h>

struct KeyState {
    bool up = false, down = false, left = false, right = false;
    bool shift = false;
    bool quit_requested = false;
    int f11_combo = 0;
    int f10_combo = 0;
};

class InputManager {
public:
    void on_event(KeySym ks, bool press);

    const KeyState& state();
    KeySym consume_press();

    void reset_state();
    void reset_f11_combo();
    void reset_f10_combo();

private:
    KeyState state_;
    std::vector<KeySym> press_queue_;
    std::chrono::steady_clock::time_point last_f11_time_;
    std::chrono::steady_clock::time_point last_f10_time_;
    int f11_combo_state_ = 0;
    int f10_combo_state_ = 0;
};
