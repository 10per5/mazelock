#include "password_overlay.hpp"
#include "cfg/config.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>

#include <X11/keysym.h>

static bool ct_equal(const std::string& a, const char* b) {
    size_t len = std::strlen(b);
    if (a.size() != len) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < len; ++i)
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return diff == 0;
}

static float rand_range(std::mt19937& rng, float lo, float hi) {
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(rng);
}

void PasswordOverlay::activate() {
    state_ = State::TYPING;
    buffer_.clear();
    flash_remaining_ = 0.0f;
    wait_remaining_ = 0.0f;
    exit_requested_ = false;
    pending_key_ = static_cast<KeySym>(0);
}

void PasswordOverlay::deactivate() {
    state_ = State::INACTIVE;
    buffer_.clear();
    flash_remaining_ = 0.0f;
    wait_remaining_ = 0.0f;
    exit_requested_ = false;
    pending_key_ = static_cast<KeySym>(0);
}

bool PasswordOverlay::handle_key(KeySym ks) {
    if (state_ == State::INACTIVE ||
        state_ == State::FLASH_SUCCESS)
        return false;

    if (state_ == State::WAITING || state_ == State::FLASH_FAILED) {
        if (state_ == State::WAITING)
            wait_remaining_ = 0.0f;
        else
            flash_remaining_ = 0.01f;
        pending_key_ = ks;
        return true;
    }

    if (ks == XK_Escape) {
        deactivate();
        return true;
    }

    if (ks == XK_Return || ks == XK_KP_Enter) {
        auto now = std::chrono::steady_clock::now();

        // Reset counter after prolonged inactivity
        if (bad_attempts_ >= 3) {
            float elapsed = std::chrono::duration<float>(now - last_attempt_time_).count();
            float cooloff = rand_range(rng_, 15.0f, 30.0f);
            if (elapsed >= cooloff)
                bad_attempts_ = 0;
        }

        last_attempt_time_ = now;

        if (cfg.quick_fail()) {
            wait_remaining_ = 0.01f;
        } else {
            wait_remaining_ = rand_range(rng_, 0.3f, 0.5f);
            if (bad_attempts_ >= 3)
                wait_remaining_ += rand_range(rng_, 5.0f, 6.0f);
        }

        state_ = State::WAITING;
        return true;
    }

    if (ks == XK_BackSpace) {
        if (!buffer_.empty())
            buffer_.pop_back();
        state_ = State::FLASH_BACKSPACE;
        flash_remaining_ = FLASH_DURATION;
        return true;
    }

    if (ks >= XK_space && ks <= XK_asciitilde) {
        if (buffer_.size() >= PASSWORD_MAX)
            return true;
        char c = static_cast<char>(ks & 0x7F);
        buffer_.push_back(c);
        state_ = State::FLASH_KEY;
        flash_remaining_ = FLASH_DURATION;
        return true;
    }

    return false;
}

void PasswordOverlay::update(float dt) {
    if (state_ == State::INACTIVE)
        return;

    if (state_ == State::WAITING) {
        wait_remaining_ -= dt;
        if (wait_remaining_ <= 0.0f) {
            wait_remaining_ = 0.0f;
            if (ct_equal(buffer_, PASSWORD)) {
                bad_attempts_ = 0;
                state_ = State::FLASH_SUCCESS;
                flash_remaining_ = SUCCESS_DURATION;
            } else {
                ++bad_attempts_;
                state_ = State::FLASH_FAILED;
                flash_remaining_ = (pending_key_ != static_cast<KeySym>(0))
                                   ? 0.01f
                                   : FAILED_DURATION;
            }
        }
        return;
    }

    if (flash_remaining_ > 0.0f) {
        flash_remaining_ -= dt;
        if (flash_remaining_ <= 0.0f) {
            flash_remaining_ = 0.0f;
            switch (state_) {
            case State::FLASH_KEY:
            case State::FLASH_BACKSPACE:
                state_ = State::TYPING;
                break;
            case State::FLASH_SUCCESS:
                exit_requested_ = true;
                break;
            case State::FLASH_FAILED:
                buffer_.clear();
                state_ = State::TYPING;
                if (pending_key_ != static_cast<KeySym>(0)) {
                    handle_key(pending_key_);
                    pending_key_ = static_cast<KeySym>(0);
                }
                break;
            default:
                break;
            }
        }
    }
}

uint32_t PasswordOverlay::overlay_color() const {
    switch (state_) {
    case State::FLASH_KEY:       return 0x888888;
    case State::FLASH_BACKSPACE: return 0x000000;
    case State::FLASH_SUCCESS:   return 0x00FF00;
    case State::FLASH_FAILED:    return 0xFF0000;
    case State::TYPING:          return 0x0000BB;
    case State::WAITING:         return 0x0000BB;
    default:                     return 0x000000;
    }
}

float PasswordOverlay::overlay_alpha() const {
    switch (state_) {
    case State::FLASH_KEY:       return 0.25f;
    case State::FLASH_BACKSPACE: return 0.25f;
    case State::FLASH_SUCCESS:   return 0.30f;
    case State::FLASH_FAILED:    return 0.30f;
    case State::TYPING:          return 0.18f;
    case State::WAITING:         return 0.18f;
    default:                     return 0.0f;
    }
}

void PasswordOverlay::draw_dots(uint32_t* buffer, int buf_w, int buf_h) const {
    if (buffer_.empty())
        return;

    static constexpr int DOT_RADIUS = 3;
    static constexpr int DOT_SPACING = 10;

    int dot_y = buf_h - 14;
    int total_w = static_cast<int>(buffer_.size()) * DOT_SPACING;
    int start_x = (buf_w - total_w) / 2;

    for (size_t i = 0; i < buffer_.size(); ++i) {
        int cx = start_x + static_cast<int>(i) * DOT_SPACING + DOT_SPACING / 2;
        for (int dy = -DOT_RADIUS; dy <= DOT_RADIUS; ++dy) {
            for (int dx = -DOT_RADIUS; dx <= DOT_RADIUS; ++dx) {
                if (dx * dx + dy * dy <= DOT_RADIUS * DOT_RADIUS + 1) {
                    int px = cx + dx;
                    int py = dot_y + dy;
                    if (px >= 0 && px < buf_w && py >= 0 && py < buf_h) {
                        uint32_t bg = buffer[py * buf_w + px];
                        int r = ((bg >> 16) & 0xFF) * 1 / 2 + 255 * 1 / 2;
                        int g = ((bg >>  8) & 0xFF) * 1 / 2 + 255 * 1 / 2;
                        int b = (bg        & 0xFF) * 1 / 2 + 255 * 1 / 2;
                        buffer[py * buf_w + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
    }
}
