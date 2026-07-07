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

void PasswordOverlay::apply_overlay(uint32_t* buffer, int width, int height) const {
    if (!buffer || width <= 0 || height <= 0)
        return;

    int oa = static_cast<int>(overlay_alpha() * 255);
    if (oa > 0) {
        int or_ = (overlay_color() >> 16) & 0xFF;
        int og = (overlay_color() >> 8)  & 0xFF;
        int ob = (overlay_color()      ) & 0xFF;
        for (int i = 0, n = width * height; i < n; ++i) {
            uint32_t c = buffer[i];
            int rr = ((c >> 16) & 0xFF) * (255 - oa) / 255 + or_ * oa / 255;
            int gg = ((c >> 8)  & 0xFF) * (255 - oa) / 255 + og * oa / 255;
            int bb = (c         & 0xFF) * (255 - oa) / 255 + ob * oa / 255;
            buffer[i] = 0xFF000000 | (rr << 16) | (gg << 8) | bb;
        }
    }
    draw_dots(buffer, width, height);
}

void PasswordOverlay::draw_dots(uint32_t* buffer, int buf_w, int buf_h) const {
    if (buffer_.empty())
        return;

    // Scale dots to match visual size across different render resolutions
    static constexpr int REF_W = 320;
    static constexpr int REF_H = 200;
    float sx = static_cast<float>(buf_w) / REF_W;
    float sy = static_cast<float>(buf_h) / REF_H;
    float s = std::min(sx, sy);

    int dot_radius = std::max(1, static_cast<int>(3.0f * s));
    int dot_spacing = std::max(1, static_cast<int>(10.0f * s));
    int dot_y = buf_h - static_cast<int>(14.0f * sy);
    int total_w = static_cast<int>(buffer_.size()) * dot_spacing;
    int start_x = (buf_w - total_w) / 2;

    for (size_t i = 0; i < buffer_.size(); ++i) {
        int cx = start_x + static_cast<int>(i) * dot_spacing + dot_spacing / 2;
        for (int dy = -dot_radius; dy <= dot_radius; ++dy) {
            for (int dx = -dot_radius; dx <= dot_radius; ++dx) {
                if (dx * dx + dy * dy <= dot_radius * dot_radius + 1) {
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
