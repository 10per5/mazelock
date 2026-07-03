#include "animal.hpp"
#include "ui/camera.hpp"
#include "graphics/texture.hpp"

#include <algorithm>
#include <cmath>

Animal::Animal(int x, int y, const Texture* sprite)
    : x_(x), y_(y), sprite_(sprite) {}

bool Animal::occupies(int x, int y) const {
    return active_ && !fleeing_ && x == x_ && y == y_;
}

uint32_t Animal::minimap_color() const {
    if (fleeing_) return 0x00000000;
    return active_ ? 0xFF44FF44 : 0x00000000;
}

void Animal::start_flee(float from_x, float from_y) {
    fleeing_ = true;
    flee_pos_x_ = x_ + 0.5f;
    flee_pos_y_ = y_ + 0.5f;

    float dx = flee_pos_x_ - from_x;
    float dy = flee_pos_y_ - from_y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0.001f) {
        flee_vx_ = (dx / len) * 2.5f;
        flee_vy_ = (dy / len) * 2.5f;
    } else {
        flee_vx_ = 0.5f;
        flee_vy_ = 0.0f;
    }

    flee_timer_ = 0.8f;
}

void Animal::update(float dt) {
    if (fleeing_) {
        flee_pos_x_ += flee_vx_ * dt;
        flee_pos_y_ += flee_vy_ * dt;
        flee_timer_ -= dt;
        if (flee_timer_ <= 0.0f) {
            active_ = false;
            fleeing_ = false;
        }
    }
    bob_ += dt * (fleeing_ ? 12.0f : 4.0f);
}

void Animal::render(uint32_t* color_buffer, const float* depth_buffer,
                    const Camera& camera,
                    int render_w, int render_h, float wall_height) const {
    if (!active_ || !sprite_) return;

    float wx = fleeing_ ? flee_pos_x_ : (x_ + 0.5f);
    float wy = fleeing_ ? flee_pos_y_ : (y_ + 0.5f);

    float tform_x, tform_y;
    if (!camera.project(wx, wy, tform_x, tform_y))
        return;

    int screen_x = static_cast<int>((render_w / 2) * (1.0f + tform_x / tform_y));
    int vh = static_cast<int>(std::abs(render_h / tform_y) * wall_height);
    int vw = vh;

    int bob_offset = static_cast<int>(std::sin(bob_) * (fleeing_ ? 6 : 2));
    int draw_start_y = -vh / 2 + render_h / 2 + bob_offset;
    if (draw_start_y < 0) draw_start_y = 0;
    int draw_end_y = vh / 2 + render_h / 2 + bob_offset;
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
