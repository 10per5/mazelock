#pragma once

// Subtle, slowly drifting per-channel colorization, meant to be applied once
// per frame to an effect's low-resolution internal cache (not the final
// upscaled framebuffer) so the extra cost is bounded by the cache size, not
// the real screen resolution.
//
// The point isn't decoration — long-running screensavers repeat the same
// RGB values at the same pixels for hours, which is exactly what causes
// burn-in on OLED/plasma panels. This slides a soft diagonal tint band
// across the frame and slowly rotates its hue, so the exact color shown at
// any given pixel keeps drifting over time even if the underlying scene
// content there doesn't change.
//
// Cost: one pass over the cache (a handful of multiply/adds per pixel, no
// per-pixel trig — cos/sin of the diagonal phase are built from cheap
// per-row/per-column tables via the angle-sum identity).

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <vector>

inline void apply_color_wash(uint32_t* pixels, int w, int h, float time_seconds,
                              float strength = 0.12f) {
    constexpr float TWO_PI = 6.28318530718f;
    constexpr float BAND_FREQ = TWO_PI / 220.0f;   // wavelength of the diagonal band
    constexpr float HUE_SPEED = TWO_PI / 2.0f;   // one full hue rotation every ~4 min
    constexpr float OFF_G = 2.09439510239f;        // 120 degrees
    constexpr float OFF_B = 4.18879020479f;        // 240 degrees

    static thread_local std::vector<float> cos_x, sin_x;
    cos_x.resize(w);
    sin_x.resize(w);
    for (int x = 0; x < w; ++x) {
        float a = BAND_FREQ * static_cast<float>(x);
        cos_x[x] = std::cos(a);
        sin_x[x] = std::sin(a);
    }

    const float cos_off_g = std::cos(OFF_G), sin_off_g = std::sin(OFF_G);
    const float cos_off_b = std::cos(OFF_B), sin_off_b = std::sin(OFF_B);

    for (int y = 0; y < h; ++y) {
        float b = BAND_FREQ * static_cast<float>(y) + time_seconds * HUE_SPEED;
        float cy = std::cos(b), sy = std::sin(b);
        uint32_t* row = &pixels[static_cast<size_t>(y) * w];

        for (int x = 0; x < w; ++x) {
            float cx = cos_x[x], sx = sin_x[x];
            float cos_phase = cx * cy - sx * sy; // cos(BAND_FREQ*x + b)
            float sin_phase = sx * cy + cx * sy; // sin(BAND_FREQ*x + b)

            float tr = 0.5f + 0.5f * cos_phase;
            float tg = 0.5f + 0.5f * (cos_phase * cos_off_g - sin_phase * sin_off_g);
            float tb = 0.5f + 0.5f * (cos_phase * cos_off_b - sin_phase * sin_off_b);

            uint32_t c = row[x];
            int r0 = static_cast<int>((c >> 16) & 0xFF);
            int g0 = static_cast<int>((c >> 8) & 0xFF);
            int b0 = static_cast<int>(c & 0xFF);

            int r = static_cast<int>(r0 * (1.0f - strength + strength * 2.0f * tr));
            int g = static_cast<int>(g0 * (1.0f - strength + strength * 2.0f * tg));
            int b_ = static_cast<int>(b0 * (1.0f - strength + strength * 2.0f * tb));

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255);
            b_ = std::clamp(b_, 0, 255);

            row[x] = 0xFF000000 | (static_cast<uint32_t>(r) << 16) |
                      (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b_);
        }
    }
}
