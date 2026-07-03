#include "pipe_effect.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <random>

static std::mt19937 rng{std::random_device{}()};

static int rand_int(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

PipeEffect::Dir PipeEffect::opposite(Dir d) {
    return static_cast<Dir>((static_cast<int>(d) + 2) % 4);
}

bool PipeEffect::is_horiz(Dir d) {
    return d == Dir::E || d == Dir::W;
}

int PipeEffect::grid_x(Dir d) {
    return (d == Dir::E) ? 1 : (d == Dir::W) ? -1 : 0;
}

int PipeEffect::grid_y(Dir d) {
    return (d == Dir::S) ? 1 : (d == Dir::N) ? -1 : 0;
}

PipeEffect::PipeEffect() {
    current_color_ = PALETTE[rand_int(0, 7)];
    tip_gx_ = 0;
    tip_gy_ = 0;
    tip_dir_ = static_cast<Dir>(rand_int(0, 3));
}

void PipeEffect::resize(int width, int height) {
    // ~30px per cell
    cell_w_ = std::max(8, width / 24);
    cell_h_ = std::max(8, height / 18);
    grid_w_ = std::max(4, width  / cell_w_);
    grid_h_ = std::max(4, height / cell_h_);
    occ_.assign(grid_w_ * grid_h_, 0);
    segments_.clear();
    dirty_ = true;

    // Seed a chain of segments so the pipe is visible immediately
    tip_gx_ = grid_w_ / 2;
    tip_gy_ = grid_h_ / 2;
    tip_dir_ = static_cast<Dir>(rand_int(0, 3));

    Dir d = tip_dir_;
    int gx = tip_gx_, gy = tip_gy_;
    int chain_len = rand_int(3, 6);

    // First segment at tip
    occ_[gy * grid_w_ + gx] |= (1 << static_cast<int>(d));
    segments_.push_back({gx, gy, d, current_color_});

    for (int i = 0; i < chain_len; ++i) {
        int nx = gx + grid_x(d);
        int ny = gy + grid_y(d);
        if (nx < 0 || nx >= grid_w_ || ny < 0 || ny >= grid_h_) break;

        // Back-connection on new cell so rendering connects
        occ_[ny * grid_w_ + nx] |= (1 << static_cast<int>(opposite(d)));
        // Forward connection on new cell
        occ_[ny * grid_w_ + nx] |= (1 << static_cast<int>(d));
        segments_.push_back({nx, ny, d, current_color_});

        gx = nx; gy = ny;
    }

    tip_gx_ = gx;
    tip_gy_ = gy;
    tip_dir_ = d;
}

bool PipeEffect::occupied(int gx, int gy, Dir d) const {
    if (gx < 0 || gx >= grid_w_ || gy < 0 || gy >= grid_h_)
        return true;
    return (occ_[gy * grid_w_ + gx] & (1 << static_cast<int>(d))) != 0;
}

void PipeEffect::grow() {
    // Try to extend the tip in current direction
    int nx = tip_gx_ + grid_x(tip_dir_);
    int ny = tip_gy_ + grid_y(tip_dir_);

    if (!occupied(nx, ny, tip_dir_) && !occupied(nx, ny, opposite(tip_dir_))) {
        uint8_t bit = 1 << static_cast<int>(tip_dir_);
        occ_[ny * grid_w_ + nx] |= bit;

        // Mark the back-connection from the new cell
        uint8_t back_bit = 1 << static_cast<int>(opposite(tip_dir_));
        occ_[ny * grid_w_ + nx] |= back_bit;

        segments_.push_back({nx, ny, tip_dir_, current_color_});
        dirty_ = true;
        tip_gx_ = nx;
        tip_gy_ = ny;
        return;
    }

    // Blocked — pick a new direction, or start a new pipe
    Dir candidates[4];
    int n = 0;
    for (int i = 0; i < 4; ++i) {
        Dir d = static_cast<Dir>(i);
        int cx = tip_gx_ + grid_x(d);
        int cy = tip_gy_ + grid_y(d);
        if (!occupied(cx, cy, d) && !occupied(cx, cy, opposite(d)))
            candidates[n++] = d;
    }

    if (n > 0) {
        tip_dir_ = candidates[rand_int(0, n - 1)];
        grow(); // recurse — immediately place the segment
        return;
    }

    // Truly stuck — start a fresh pipe somewhere else
    for (int attempt = 0; attempt < 100; ++attempt) {
        int gx = rand_int(1, grid_w_ - 2);
        int gy = rand_int(1, grid_h_ - 2);
        Dir d = static_cast<Dir>(rand_int(0, 3));
        if (!occupied(gx, gy, d)) {
            current_color_ = PALETTE[rand_int(0, 7)];
            uint8_t bit = 1 << static_cast<int>(d);
            occ_[gy * grid_w_ + gx] |= bit;
            segments_.push_back({gx, gy, d, current_color_});
            dirty_ = true;
            tip_gx_ = gx;
            tip_gy_ = gy;
            tip_dir_ = d;
            return;
        }
    }
}

void PipeEffect::update(float dt) {
    if (grid_w_ == 0) return;

    timer_ += dt;
    while (timer_ >= interval_) {
        timer_ -= interval_;
        grow();
    }
}

void PipeEffect::draw_segment(uint32_t* buf, int bw, int bh, const Segment& seg) {
    int tube_w = std::max(2, cell_w_ * 5 / 8);
    int tube_h = std::max(2, cell_h_ * 5 / 8);
    int hw = tube_w / 2, hh = tube_h / 2;
    int half_cw = cell_w_ / 2, half_ch = cell_h_ / 2;

    int px = seg.gx * cell_w_;
    int py = seg.gy * cell_h_;
    int cx = px + half_cw;
    int cy = py + half_ch;

    uint32_t col = seg.color;
    int r = (col >> 16) & 0xFF;
    int g = (col >>  8) & 0xFF;
    int b = (col >>  0) & 0xFF;

    if (is_horiz(seg.dir)) {
        int sign = (seg.dir == Dir::E) ? 1 : -1;
        for (int dy = -hh; dy <= hh; ++dy) {
            float shade = 1.0f - 0.4f * std::abs(dy) / (hh + 1);
            int rr = std::min(255, static_cast<int>(r * shade));
            int gg = std::min(255, static_cast<int>(g * shade));
            int bb = std::min(255, static_cast<int>(b * shade));
            int sy = cy + dy;
            int cr = rr, cg = gg, cb = bb;
            auto tube = [&](int sx0, int step, bool skip) {
                for (int dx = skip ? step : 0; dx * step >= 0 && dx * step <= half_cw; dx += step) {
                    int sx = sx0 + dx;
                    if (sx >= 0 && sx < bw && sy >= 0 && sy < bh)
                        buf[sy * bw + sx] = 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                }
            };
            tube(cx, sign, false);
            tube(cx, -sign, true);
        }
    } else {
        int sign = (seg.dir == Dir::S) ? 1 : -1;
        for (int dx = -hw; dx <= hw; ++dx) {
            float shade = 1.0f - 0.4f * std::abs(dx) / (hw + 1);
            int rr = std::min(255, static_cast<int>(r * shade));
            int gg = std::min(255, static_cast<int>(g * shade));
            int bb = std::min(255, static_cast<int>(b * shade));
            int sx = cx + dx;
            int cr = rr, cg = gg, cb = bb;
            auto tube = [&](int sy0, int step, bool skip) {
                for (int dy = skip ? step : 0; dy * step >= 0 && dy * step <= half_ch; dy += step) {
                    int sy = sy0 + dy;
                    if (sx >= 0 && sx < bw && sy >= 0 && sy < bh)
                        buf[sy * bw + sx] = 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                }
            };
            tube(cy, sign, false);
            tube(cy, -sign, true);
        }
    }

    // Joint circle at centre
    int rad = std::max(hw, hh);
    for (int dy = -rad; dy <= rad; ++dy) {
        for (int dx = -rad; dx <= rad; ++dx) {
            if (dx * dx + dy * dy > rad * rad) continue;
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / (rad + 1);
            float shade = 1.0f - 0.5f * dist;
            int rr = std::min(255, static_cast<int>(r * shade));
            int gg = std::min(255, static_cast<int>(g * shade));
            int bb = std::min(255, static_cast<int>(b * shade));
            int sx = cx + dx;
            int sy = cy + dy;
            if (sx >= 0 && sx < bw && sy >= 0 && sy < bh)
                buf[sy * bw + sx] = 0xFF000000 | (rr << 16) | (gg << 8) | bb;
        }
    }
}

void PipeEffect::render(uint32_t* buffer, int width, int height) {
    if (grid_w_ == 0) {
        resize(RENDER_W, RENDER_H);
        cache_.resize(static_cast<size_t>(RENDER_W) * RENDER_H);
        cache_w_ = RENDER_W;
        cache_h_ = RENDER_H;
        dirty_ = true;
    }

    if (dirty_) {
        std::fill(cache_.begin(), cache_.end(), 0xFF111822);
        for (const auto& seg : segments_)
            draw_segment(cache_.data(), cache_w_, cache_h_, seg);
        dirty_ = false;
    }

    // Nearest-neighbour upscale from 320x200 cache to target buffer
    for (int y = 0; y < height; ++y) {
        int sy = y * cache_h_ / height;
        const uint32_t* src = &cache_[sy * cache_w_];
        for (int x = 0; x < width; ++x)
            buffer[y * width + x] = src[x * cache_w_ / width];
    }
}
