#include "flower_magnifier.hpp"

#include "graphics/texture_manager.hpp"
#include "graphics/texture.hpp"

#include <algorithm>
#include <cmath>
#include <random>

static std::mt19937 rng{std::random_device{}()};

static int rand_int(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

static float smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

FlowerMagnifierEffect::FlowerMagnifierEffect(const TextureManager& textures)
    : textures_(textures) {
    cache_.assign(static_cast<size_t>(CACHE_W) * CACHE_H, 0xFF000000);
    flower_index_ = rand_int(0, textures_.flower_count() - 1);
}

void FlowerMagnifierEffect::pick_next_flower() {
    if (textures_.flower_count() <= 1) return;
    int next;
    do {
        next = rand_int(0, textures_.flower_count() - 1);
    } while (next == flower_index_);
    flower_index_ = next;
}

void FlowerMagnifierEffect::update(float dt) {
    phase_time_ += dt;
    bob_phase_ += dt;

    switch (phase_) {
        case Phase::FAR:
            if (phase_time_ >= FAR_HOLD_TIME) {
                phase_ = Phase::ZOOM_IN;
                phase_time_ = 0.0f;
            }
            break;
        case Phase::ZOOM_IN:
            if (phase_time_ >= ZOOM_TIME) {
                phase_ = Phase::HOLD;
                phase_time_ = 0.0f;
            }
            break;
        case Phase::HOLD:
            if (phase_time_ >= FULL_HOLD_TIME) {
                phase_ = Phase::ZOOM_OUT;
                phase_time_ = 0.0f;
            }
            break;
        case Phase::ZOOM_OUT:
            if (phase_time_ >= ZOOM_TIME) {
                phase_ = Phase::FAR;
                phase_time_ = 0.0f;
                pick_next_flower();
            }
            break;
    }

    // Rebuild the low-res composite once per frame here, not in render() —
    // render() is called once per monitor, update() once per frame.
    rebuild_cache();
}

void FlowerMagnifierEffect::rebuild_cache() {
    // Soft vertical gradient background (a couple of muted greenhouse tones),
    // computed per row rather than per pixel.
    for (int y = 0; y < CACHE_H; ++y) {
        float t = static_cast<float>(y) / CACHE_H;
        int r = static_cast<int>(18 + 14 * t);
        int g = static_cast<int>(30 + 24 * t);
        int b = static_cast<int>(20 + 16 * t);
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
        std::fill(&cache_[y * CACHE_W], &cache_[y * CACHE_W] + CACHE_W, color);
    }

    // Eased zoom progress: 0 = far/tiny, 1 = fully magnified.
    float t = 0.0f;
    switch (phase_) {
        case Phase::FAR:      t = 0.0f; break;
        case Phase::ZOOM_IN:  t = smoothstep(phase_time_ / ZOOM_TIME); break;
        case Phase::HOLD:     t = 1.0f; break;
        case Phase::ZOOM_OUT: t = 1.0f - smoothstep(phase_time_ / ZOOM_TIME); break;
    }

    // "Full" size fills the viewport height while staying square, so the
    // flower never gets stretched into a wide oval.
    const float far_size  = 26.0f;
    const float full_size = static_cast<float>(CACHE_H) * 0.92f;
    float size = far_size + (full_size - far_size) * t;

    // Gentle pulse while fully zoomed so it doesn't feel frozen.
    if (phase_ == Phase::HOLD)
        size += std::sin(phase_time_ * 1.3f) * (full_size * 0.02f);

    // Bob fades out as it zooms in — only visible while "far away".
    float bob_amp = 6.0f * (1.0f - t);
    float cy = CACHE_H / 2.0f + std::sin(bob_phase_ * 1.4f) * bob_amp;
    float cx = CACHE_W / 2.0f;

    const Texture& tex = textures_.flower(flower_index_);
    int tex_w = tex.width();
    int tex_h = tex.height();
    if (tex_w <= 0 || tex_h <= 0) return;

    int isize = std::max(1, static_cast<int>(size));
    int left = static_cast<int>(cx - isize / 2.0f);
    int top  = static_cast<int>(cy - isize / 2.0f);

    int x0 = std::max(0, left);
    int y0 = std::max(0, top);
    int x1 = std::min(CACHE_W - 1, left + isize);
    int y1 = std::min(CACHE_H - 1, top + isize);

    for (int py = y0; py <= y1; ++py) {
        int ty = (py - top) * tex_h / isize;
        if (ty < 0 || ty >= tex_h) continue;
        for (int px = x0; px <= x1; ++px) {
            int tx = (px - left) * tex_w / isize;
            if (tx < 0 || tx >= tex_w) continue;
            uint32_t c = tex.pixel(tx, ty);
            if ((c & 0xFF000000) == 0) continue; // transparent — keep background
            cache_[py * CACHE_W + px] = c;
        }
    }
}

void FlowerMagnifierEffect::render(uint32_t* buffer, int width, int height) {
    // Composite is rebuilt in update() (once per frame); render() only
    // computes the viewport and blits — called once per monitor.

    // Centred 16:9 viewport inside whatever aspect ratio the real output is.
    const float target_ar = 16.0f / 9.0f;
    float buf_ar = static_cast<float>(width) / static_cast<float>(height);

    int vw, vh, vx0, vy0;
    if (buf_ar > target_ar) {
        vh = height;
        vw = static_cast<int>(height * target_ar);
        vx0 = (width - vw) / 2;
        vy0 = 0;
    } else {
        vw = width;
        vh = static_cast<int>(width / target_ar);
        vx0 = 0;
        vy0 = (height - vh) / 2;
    }
    vw = std::max(1, vw);
    vh = std::max(1, vh);

    // Letterbox/pillarbox bars.
    std::fill(buffer, buffer + static_cast<size_t>(width) * height, 0xFF05070A);

    if (lut_w_ != vw) {
        x_lut_.resize(vw);
        for (int x = 0; x < vw; ++x)
            x_lut_[x] = x * CACHE_W / vw;
        lut_w_ = vw;
    }
    if (lut_h_ != vh) {
        y_lut_.resize(vh);
        for (int y = 0; y < vh; ++y)
            y_lut_[y] = y * CACHE_H / vh;
        lut_h_ = vh;
    }

    for (int y = 0; y < vh; ++y) {
        const uint32_t* src = &cache_[y_lut_[y] * CACHE_W];
        uint32_t* dst = &buffer[(vy0 + y) * width + vx0];
        for (int x = 0; x < vw; ++x)
            dst[x] = src[x_lut_[x]];
    }
}
