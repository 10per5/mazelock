#include "input_manager.hpp"

void InputManager::on_event(KeySym ks, bool press) {
    if (press)
        press_queue_.push_back(ks);

    switch (ks) {
    case XK_F1:       state_.f1_pressed = press; break;
    case XK_Up:       state_.up    = press; break;
    case XK_Down:     state_.down  = press; break;
    case XK_Left:     state_.left  = press; break;
    case XK_Right:    state_.right = press; break;
    case XK_Shift_L:  case XK_Shift_R: state_.shift = press; break;
    case XK_F12:
        if (press) state_.quit_requested = true;
        break;
    case XK_F11:
        if (press) {
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - last_f11_time_).count();
            last_f11_time_ = now;
            if (elapsed < 0.5f && elapsed >= 0.0f)
                ++f11_combo_state_;
            else
                f11_combo_state_ = 1;
        }
        break;
    case XK_F10:
        if (press) {
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - last_f10_time_).count();
            last_f10_time_ = now;
            if (elapsed < 0.5f && elapsed >= 0.0f)
                ++f10_combo_state_;
            else
                f10_combo_state_ = 1;
        }
        break;
    default: break;
    }
}

const KeyState& InputManager::state() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - last_f11_time_).count();
    if (elapsed >= 0.5f && f11_combo_state_ > 0)
        f11_combo_state_ = 0;
    state_.f11_combo = f11_combo_state_;
    elapsed = std::chrono::duration<float>(now - last_f10_time_).count();
    if (elapsed >= 0.5f && f10_combo_state_ > 0)
        f10_combo_state_ = 0;
    state_.f10_combo = f10_combo_state_;
    return state_;
}

KeySym InputManager::consume_press() {
    if (press_queue_.empty()) return NoSymbol;
    KeySym r = press_queue_.front();
    press_queue_.erase(press_queue_.begin());
    return r;
}

void InputManager::reset_state() {
    state_ = KeyState{};
    press_queue_.clear();
    f11_combo_state_ = 0;
    f10_combo_state_ = 0;
}

void InputManager::reset_f11_combo() {
    f11_combo_state_ = 0;
    state_.f11_combo = 0;
}

void InputManager::reset_f10_combo() {
    f10_combo_state_ = 0;
    state_.f10_combo = 0;
}
