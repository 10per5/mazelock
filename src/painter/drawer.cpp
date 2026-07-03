#include "drawer.hpp"
#include "graphics/framebuffer.hpp"
#include "graphics/raycaster.hpp"

#include <algorithm>

void Drawer::frame(Framebuffer& fb, Raycaster& raycaster,
                   float wall_height,
                   uint32_t overlay_color, float overlay_alpha) {
    const auto& buf = raycaster.color_buffer();
    int rw = raycaster.render_width();
    int rh = raycaster.render_height();
    int fb_w = fb.xres();
    int fb_h = fb.yres();

    if (static_cast<int>(row_buf_.size()) < fb_w)
        row_buf_.resize(fb_w);

    int oa = static_cast<int>(overlay_alpha * 255);
    int or_ = (overlay_color >> 16) & 0xFF;
    int og = (overlay_color >> 8)  & 0xFF;
    int ob = (overlay_color)       & 0xFF;

    for (int sy = 0; sy < rh; ++sy) {
        const uint32_t* src = &buf[sy * rw];
        if (oa > 0) {
            for (int x = 0; x < fb_w; ++x) {
                uint32_t c = src[x * rw / fb_w];
                int rr = ((c >> 16) & 0xFF) * (255 - oa) / 255 + or_ * oa / 255;
                int gg = ((c >> 8)  & 0xFF) * (255 - oa) / 255 + og * oa / 255;
                int bb = (c        & 0xFF) * (255 - oa) / 255 + ob * oa / 255;
                row_buf_[x] = 0xFF000000 | (rr << 16) | (gg << 8) | bb;
            }
        } else {
            for (int x = 0; x < fb_w; ++x) {
                row_buf_[x] = src[x * rw / fb_w];
            }
        }
        int y0 = sy * fb_h / rh;
        int y1 = (sy + 1) * fb_h / rh;
        for (int y = y0; y < y1; ++y)
            fb.write_row(y, row_buf_.data(), fb_w);
    }
}
