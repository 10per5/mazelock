#include "heart_display.hpp"

#include "graphics/framebuffer.hpp"
#include "graphics/texture_manager.hpp"
#include "security/password_overlay.hpp"

static constexpr int EV_PER_PASS = 42;
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

static int quadrant(int tx, int ty, int mid) {
    if (tx > mid) {
        if (ty < mid) return 1;
        if (ty > mid) return 3;
    } else if (tx < mid) {
        if (ty < mid) return 0;
        if (ty > mid) return 2;
    }
    return -1;
}

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

    static const int qcx[] = {4, 12, 4, 12};
    static const int qcy[] = {4, 4, 12, 12};

    for (int hi = 0; hi < 3; ++hi) {
        int hx = sx + hi * (hw + gap);
        int wh_v_ev = (2 - hi) * 2;
        int wh_h_ev = wh_v_ev + 1;
        int ph2_base = 6 + (2 - hi) * 12;

        for (int py = 0; py < hh; ++py) {
            int ty = py / scale;
            if (ty >= th) continue;

            for (int px = 0; px < hw; ++px) {
                int tx = px / scale;
                if (tx >= tw) continue;

                uint32_t orig = tex.pixel(tx, ty);
                if ((orig & 0xFF000000) == 0) continue;

                uint32_t a = orig & 0xFF000000;
                int qi = quadrant(tx, ty, mid);
                bool on_crack = false;

                if (qi >= 0) {
                    int sub_v_ev = ph2_base + 3 * qi;
                    int sub_h_ev = sub_v_ev + 1;
                    int paint_ev = sub_v_ev + 2;

                    if (ba > paint_ev) {
                        uint32_t rgb = PASS_COLORS[pass] & 0x00FFFFFF;
                        fb.put_pixel(hx + px, sy + py, a | rgb);
                        continue;
                    }

                    if (ba > sub_v_ev && tx == qcx[qi]) on_crack = true;
                    if (ba > sub_h_ev && ty == qcy[qi]) on_crack = true;
                }

                if (ba > wh_v_ev && tx == mid) on_crack = true;
                if (ba > wh_h_ev && ty == mid) on_crack = true;

                if (on_crack) continue; // transparent — shows background through

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
