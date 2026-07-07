#include "heart_display.hpp"

#include "graphics/framebuffer.hpp"
#include "graphics/texture_manager.hpp"
#include "security/password_overlay.hpp"

static constexpr int EV_PER_PASS = 10; // 3 cracks + 6 half recolor + 1 full recolor
static constexpr int NUM_PASSES = 5;
static const uint32_t PASS_COLORS[NUM_PASSES] = {
    0xFF000000, // black
    0xFFFFE000, // yellow
    0xFF00E000, // green
    0xFFC000FF, // purple
    0xFFFFFFFF, // white
};

HeartDisplay::HeartDisplay(PasswordOverlay& overlay, TextureManager& texman)
    : overlay_(overlay), texman_(texman) {}

void HeartDisplay::draw(Framebuffer& fb) const {
    const auto& tex = texman_.heart();
    int tw = tex.width(), th = tex.height();
    if (tw == 0 || th == 0) return;

    int raw = overlay_.bad_attempts();
    if (raw <= 0) return;

    int total_events = NUM_PASSES * EV_PER_PASS;
    if (raw > total_events) raw = total_events;

    int ev_idx = raw - 1;
    int pass = ev_idx / EV_PER_PASS;
    if (pass >= NUM_PASSES) pass = NUM_PASSES - 1;
    int ba = (ev_idx % EV_PER_PASS) + 1;

    int fb_w = fb.xres(), fb_h = fb.yres();
    const int scale = 3, gap = 4;
    int hw = tw * scale, hh = th * scale;
    int total = 3 * hw + 2 * gap;
    int sx = (fb_w - total) / 2, sy = fb_h - hh - 8;

    int mid = tw / 2;

    // thunder-like zigzag: crack and half-edge jag 1px at random-seeming rows
    static const int8_t zigzag[8] = {0, 0, -1, 0, 1, 0, -1, 0};

    for (int hi = 0; hi < 3; ++hi) {
        int hx = sx + hi * (hw + gap);

        // ba=1-3: cracks (rightmost → middle → leftmost)
        int crack_ev = 2 - hi; // hi=2→0, hi=1→1, hi=0→2

        // ba=10: full recolor, remove cracks
        bool fully_recolored = (ba >= 10);

        for (int py = 0; py < hh; ++py) {
            int ty = py / scale;
            if (ty >= th) continue;

            int dz = zigzag[ty & 7];

            for (int px = 0; px < hw; ++px) {
                int tx = px / scale;
                if (tx >= tw) continue;

                uint32_t orig = tex.pixel(tx, ty);
                if ((orig & 0xFF000000) == 0) continue;

                uint32_t a = orig & 0xFF000000;

                if (fully_recolored) {
                    uint32_t rgb = PASS_COLORS[pass] & 0x00FFFFFF;
                    fb.put_pixel(hx + px, sy + py, a | rgb);
                    continue;
                }

                bool on_crack = false;
                if (ba > crack_ev && tx == mid + dz) on_crack = true;

                if (!on_crack) {
                    // ba=4-9: half-heart recolor, right-to-left, right-half first
                    int half_ev = -1;
                    if (hi == 2)      half_ev = (tx > mid + dz) ? 3 : 4;
                    else if (hi == 1) half_ev = (tx > mid + dz) ? 5 : 6;
                    else              half_ev = (tx > mid + dz) ? 7 : 8;

                    if (half_ev >= 0 && ba > half_ev) {
                        uint32_t rgb = PASS_COLORS[pass] & 0x00FFFFFF;
                        fb.put_pixel(hx + px, sy + py, a | rgb);
                        continue;
                    }
                }

                if (on_crack) continue;

                uint32_t c;
                if (pass > 0)
                    c = a | (PASS_COLORS[pass - 1] & 0x00FFFFFF);
                else
                    c = orig;

                fb.put_pixel(hx + px, sy + py, c);
            }
        }
    }
}
