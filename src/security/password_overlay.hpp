#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <string>

#include <X11/Xlib.h>

#ifndef PASSWORD
#define PASSWORD "world90s"
#endif

class PasswordOverlay {
public:
    enum class State {
        INACTIVE,
        TYPING,
        WAITING,
        FLASH_KEY,
        FLASH_BACKSPACE,
        FLASH_SUCCESS,
        FLASH_FAILED,
    };

    void activate();
    void deactivate();
    bool active() const { return state_ != State::INACTIVE; }
    bool exit_requested() const { return exit_requested_; }

    bool handle_key(KeySym ks);
    void update(float dt);

    uint32_t overlay_color() const;
    float overlay_alpha() const;
    State current_state() const { return state_; }
    int bad_attempts() const { return bad_attempts_; }
    void reset_bad_attempts() { bad_attempts_ = 0; }

    void draw_dots(uint32_t* buffer, int buf_w, int buf_h) const;

private:
    State state_ = State::INACTIVE;
    std::string buffer_;
    float flash_remaining_ = 0.0f;
    bool exit_requested_ = false;

    float wait_remaining_ = 0.0f;
    int bad_attempts_ = 0;
    std::chrono::steady_clock::time_point last_attempt_time_;
    std::mt19937 rng_{std::random_device{}()};

    KeySym pending_key_ = static_cast<KeySym>(0);

    static constexpr float FLASH_DURATION = 0.03f;
    static constexpr float SUCCESS_DURATION = 0.5f;
    static constexpr float FAILED_DURATION = 1.5f;
};
