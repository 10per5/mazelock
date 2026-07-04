#include "animal.hpp"
#include "algorithm/maze.hpp"
#include "ui/camera.hpp"
#include "graphics/texture.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

Animal::Animal(int x, int y, const Texture* sprite)
    : x_(x), y_(y), sprite_(sprite) {}

bool Animal::occupies(int x, int y) const {
    return active_ && !fleeing_ && x == x_ && y == y_;
}

float Animal::world_x() const { return fleeing_ ? flee_pos_x_ : (x_ + 0.5f); }
float Animal::world_y() const { return fleeing_ ? flee_pos_y_ : (y_ + 0.5f); }

uint32_t Animal::minimap_color() const {
    if (fleeing_) return 0x00000000;
    return active_ ? 0xFF44FF44 : 0x00000000;
}

void Animal::start_flee(MazeGenerator& maze, float from_x, float from_y) {
    fleeing_ = true;
    flee_pos_x_ = x_ + 0.5f;
    flee_pos_y_ = y_ + 0.5f;

    static constexpr int dx[4] = { 0, 1, 0, -1 };
    static constexpr int dy[4] = {-1, 0, 1,  0 };

    flee_path_.clear();
    flee_path_.push_back({x_, y_});

    int cx = x_, cy = y_;
    int steps = 3 + (std::rand() % 3);

    for (int i = 0; i < steps; ++i) {
        int best_dir = -1;
        float best_score = -999.0f;

        for (int d = 0; d < 4; ++d) {
            if (maze.is_wall(cx, cy, d)) continue;
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            // Don't backtrack
            if (flee_path_.size() >= 2) {
                auto& prev = flee_path_[flee_path_.size() - 2];
                if (nx == prev.first && ny == prev.second) continue;
            }

            float score = (nx - from_x) * (nx - from_x)
                        + (ny - from_y) * (ny - from_y);
            if (score > best_score) {
                best_score = score;
                best_dir = d;
            }
        }

        if (best_dir < 0) break;

        cx += dx[best_dir];
        cy += dy[best_dir];
        flee_path_.push_back({cx, cy});
    }

    flee_path_idx_ = 1;

    if (flee_path_.size() <= 2) {
        fallback_ = true;
        flee_timer_ = 0.6f;
        flee_timer_max_ = 0.6f;
        flee_pos_x_ = x_ + 0.5f;
        flee_pos_y_ = y_ + 0.5f;
    }
}

void Animal::update(float dt) {
    if (fleeing_) {
        if (fallback_) {
            flee_timer_ -= dt;
            if (flee_timer_ <= 0.0f) {
                active_ = false;
                fleeing_ = false;
                fallback_ = false;
            }
        } else if (flee_path_idx_ < flee_path_.size()) {
            float tx = flee_path_[flee_path_idx_].first + 0.5f;
            float ty = flee_path_[flee_path_idx_].second + 0.5f;

            float dx = tx - flee_pos_x_;
            float dy = ty - flee_pos_y_;
            float dist = std::sqrt(dx * dx + dy * dy);

            float speed = 4.0f;
            float step = speed * dt;

            if (dist <= step) {
                flee_pos_x_ = tx;
                flee_pos_y_ = ty;
                ++flee_path_idx_;
            } else {
                flee_pos_x_ += (dx / dist) * step;
                flee_pos_y_ += (dy / dist) * step;
            }
        } else {
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

    if (fallback_) {
        float t = 1.0f - flee_timer_ / flee_timer_max_;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        int dw = static_cast<int>((draw_end_x - draw_start_x) * t * 0.5f / 2);
        int dh = static_cast<int>((draw_end_y - draw_start_y) * t * 0.5f / 2);
        draw_start_x += dw;
        draw_end_x -= dw;
        draw_start_y += dh;
        draw_end_y -= dh;
    }

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
