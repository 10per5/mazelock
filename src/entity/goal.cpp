#include "goal.hpp"
#include "ui/camera.hpp"
#include "graphics/texture.hpp"

#include <algorithm>
#include <cmath>

Goal::Goal(int x, int y, uint32_t color, const Texture* sprite)
    : x_(x), y_(y), color_(color), sprite_(sprite) {}

bool Goal::occupies(int x, int y) const {
    return x == x_ && y == y_;
}

uint32_t Goal::minimap_color() const {
    return color_;
}

void Goal::render(uint32_t* color_buffer, const float* depth_buffer,
                  const Camera& camera,
                  int render_w, int render_h, float wall_height) const {
    if (!sprite_) return;

    float tform_x, tform_y;
    if (!camera.project(x_ + 0.5f, y_ + 0.5f, tform_x, tform_y))
        return;

    int screen_x = static_cast<int>((render_w / 2) * (1.0f + tform_x / tform_y));
    int vh = static_cast<int>(std::abs(render_h / tform_y) * wall_height);
    int vw = vh;

    int draw_start_y = -vh / 2 + render_h / 2;
    if (draw_start_y < 0) draw_start_y = 0;
    int draw_end_y = vh / 2 + render_h / 2;
    if (draw_end_y >= render_h) draw_end_y = render_h - 1;

    int draw_start_x = screen_x - vw / 2;
    if (draw_start_x < 0) draw_start_x = 0;
    int draw_end_x = screen_x + vw / 2;
    if (draw_end_x >= render_w) draw_end_x = render_w - 1;

    int tw = sprite_->width();
    int th = sprite_->height();
    if (tw == 0 || th == 0) return;

    for (int sy = draw_start_y; sy <= draw_end_y; ++sy) {
        for (int sx = draw_start_x; sx <= draw_end_x; ++sx) {
            if (depth_buffer[sx] < tform_y) continue;

            int tx = (sx - draw_start_x) * tw / (draw_end_x - draw_start_x + 1);
            int ty = (sy - draw_start_y) * th / (draw_end_y - draw_start_y + 1);
            uint32_t c = sprite_->pixel(tx, ty);

            if ((c & 0xFF000000) == 0) continue;
            color_buffer[sy * render_w + sx] = c;
        }
    }
}
