#include "fish_aquarium.hpp"
#include "color_wash.hpp"

#include <algorithm>
#include <cmath>
#include <random>

static std::mt19937 rng{std::random_device{}()};

static int rand_int(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

static float rand_float(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng);
}

static constexpr uint32_t FISH_BODY_COLORS[] = {
    0xFFFF8C1A, // orange
    0xFFFFD23F, // yellow
    0xFF3FA7D6, // blue
    0xFFE84A5F, // red
    0xFF9B5DE5, // purple
};

FishAquariumEffect::FishAquariumEffect() = default;

void FishAquariumEffect::init_scene() {
    cache_.assign(static_cast<size_t>(RENDER_W) * RENDER_H, 0xFF04223A);

    // Fish — 6 of them, varied size/speed/depth.
    fish_.resize(6);
    for (auto& f : fish_)
        respawn_fish(f, /*at_edge=*/false);

    // Kelp fronds along the bottom.
    kelp_.resize(5);
    for (int i = 0; i < 5; ++i) {
        kelp_[i].base_x = 20 + i * (RENDER_W - 40) / 4 + rand_int(-10, 10);
        kelp_[i].height = rand_int(35, 60);
        kelp_[i].sway_phase = rand_float(0.0f, 6.28f);
    }

    // Bubbles.
    bubbles_.resize(8);
    for (auto& b : bubbles_)
        respawn_bubble(b);

    // Pebbles on the sandy floor — computed once, static thereafter.
    pebbles_.clear();
    int floor_y = RENDER_H - 14;
    for (int i = 0; i < 18; ++i) {
        Pebble p;
        p.x = rand_int(4, RENDER_W - 4);
        p.y = floor_y + rand_int(2, 10);
        p.radius = rand_int(1, 3);
        int shade = rand_int(90, 140);
        p.color = 0xFF000000 | (shade << 16) | ((shade * 3 / 4) << 8) | (shade / 2);
        pebbles_.push_back(p);
    }

    initialized_ = true;
}

void FishAquariumEffect::respawn_fish(Fish& f, bool at_edge) {
    bool goes_right = rand_int(0, 1) == 0;
    f.speed = rand_float(14.0f, 34.0f) * (goes_right ? 1.0f : -1.0f);
    f.x = at_edge ? (goes_right ? -12.0f : RENDER_W + 12.0f)
                  : rand_float(0.0f, static_cast<float>(RENDER_W));
    f.base_y = rand_float(20.0f, RENDER_H - 30.0f);
    f.bob_phase = rand_float(0.0f, 6.28f);
    f.bob_freq = rand_float(1.2f, 2.2f);
    f.bob_amp = rand_float(3.0f, 7.0f);
    f.len = rand_int(7, 13);
    constexpr int kBodyColorCount = static_cast<int>(sizeof(FISH_BODY_COLORS) / sizeof(FISH_BODY_COLORS[0]));
    f.body_color = FISH_BODY_COLORS[rand_int(0, kBodyColorCount - 1)];
    // Fin a shade darker than the body.
    int r = ((f.body_color >> 16) & 0xFF) * 2 / 3;
    int g = ((f.body_color >> 8)  & 0xFF) * 2 / 3;
    int b = (f.body_color         & 0xFF) * 2 / 3;
    f.fin_color = 0xFF000000 | (r << 16) | (g << 8) | b;
}

void FishAquariumEffect::respawn_bubble(Bubble& b) {
    b.x = rand_float(10.0f, RENDER_W - 10.0f);
    b.y = rand_float(RENDER_H - 15.0f, RENDER_H + 20.0f);
    b.speed = rand_float(12.0f, 26.0f);
    b.radius = rand_int(1, 2);
}

void FishAquariumEffect::update(float dt) {
    if (!initialized_)
        init_scene();

    time_ += dt;

    for (auto& f : fish_) {
        f.x += f.speed * dt;
        if (f.speed > 0.0f && f.x > RENDER_W + f.len + 4)
            respawn_fish(f, /*at_edge=*/true);
        else if (f.speed < 0.0f && f.x < -f.len - 4)
            respawn_fish(f, /*at_edge=*/true);
    }

    for (auto& b : bubbles_) {
        b.y -= b.speed * dt;
        b.x += std::sin(time_ * 1.5f + b.y * 0.05f) * 4.0f * dt;
        if (b.y < 4.0f)
            respawn_bubble(b);
    }

    // Rebuild the low-res scene once per frame here, not in render() — this
    // gets called once regardless of how many monitors are being driven.
    // Vertical water gradient, computed per-row (not per-pixel).
    for (int y = 0; y < RENDER_H; ++y) {
        float t = static_cast<float>(y) / RENDER_H;
        int r = static_cast<int>(2 + 6 * t);
        int g = static_cast<int>(30 + 50 * t);
        int b = static_cast<int>(58 + 70 * t);
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
        std::fill(&cache_[y * RENDER_W], &cache_[y * RENDER_W] + RENDER_W, color);
    }

    // Sandy floor band.
    int floor_y = RENDER_H - 14;
    for (int y = floor_y; y < RENDER_H; ++y)
        std::fill(&cache_[y * RENDER_W], &cache_[y * RENDER_W] + RENDER_W, 0xFFC2A66B);

    for (const auto& p : pebbles_) {
        for (int dy = -p.radius; dy <= p.radius; ++dy) {
            for (int dx = -p.radius; dx <= p.radius; ++dx) {
                if (dx * dx + dy * dy > p.radius * p.radius) continue;
                int px = p.x + dx, py = p.y + dy;
                if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                    cache_[py * RENDER_W + px] = p.color;
            }
        }
    }

    for (const auto& k : kelp_)
        draw_kelp(k);

    for (const auto& b : bubbles_) {
        int bx = static_cast<int>(b.x), by = static_cast<int>(b.y);
        for (int dy = -b.radius; dy <= b.radius; ++dy) {
            for (int dx = -b.radius; dx <= b.radius; ++dx) {
                if (dx * dx + dy * dy > b.radius * b.radius) continue;
                int px = bx + dx, py = by + dy;
                if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                    cache_[py * RENDER_W + px] = 0x80CFEFFF;
            }
        }
    }

    for (const auto& f : fish_)
        draw_fish(f);

    // Slow, subtle color drift across the whole scene — mitigates burn-in
    // from the same colors sitting in the same spot for hours. Cheap: runs
    // once per frame over the low-res cache, not the real screen size.
    apply_color_wash(cache_.data(), RENDER_W, RENDER_H, time_, 0.10f);
}

void FishAquariumEffect::draw_kelp(const Kelp& k) {
    int segments = std::max(4, k.height / 8);
    float step = static_cast<float>(k.height) / segments;

    for (int s = 0; s < segments; ++s) {
        float t = static_cast<float>(s) / segments; // 0 at base, 1 at tip
        float amp = 3.0f + 5.0f * t; // sways more near the tip
        float sway = std::sin(time_ * 0.9f + k.sway_phase + t * 2.0f) * amp;

        int y = RENDER_H - 14 - static_cast<int>(s * step);
        int x = k.base_x + static_cast<int>(sway);

        int shade = 60 + static_cast<int>(40.0f * (1.0f - t));
        uint32_t color = 0xFF000000 | (static_cast<uint32_t>(shade / 3) << 16) |
                          (static_cast<uint32_t>(shade + 40) << 8) | static_cast<uint32_t>(shade / 3);

        int half_w = std::max(1, 2 - s / (segments / 2 + 1));
        for (int dx = -half_w; dx <= half_w; ++dx) {
            int px = x + dx, py = y;
            if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                cache_[py * RENDER_W + px] = color;
        }
    }
}

void FishAquariumEffect::draw_fish(const Fish& f) {
    float y = f.base_y + std::sin(time_ * f.bob_freq + f.bob_phase) * f.bob_amp;
    int cx = static_cast<int>(f.x);
    int cy = static_cast<int>(y);
    bool facing_right = f.speed > 0.0f;
    int len = f.len;
    int half_h = std::max(3, len / 2);

    // Body — flattened ellipse.
    for (int dy = -half_h; dy <= half_h; ++dy) {
        float ratio = 1.0f - (static_cast<float>(dy * dy) / static_cast<float>(half_h * half_h + 1));
        if (ratio < 0.0f) continue;
        int half_span = static_cast<int>(len * std::sqrt(ratio));
        int py = cy + dy;
        if (py < 0 || py >= RENDER_H) continue;
        int x0 = std::max(0, cx - half_span);
        int x1 = std::min(RENDER_W - 1, cx + half_span);
        for (int px = x0; px <= x1; ++px)
            cache_[py * RENDER_W + px] = f.body_color;
    }

    // Tail fin — small triangle on the trailing side.
    int tail_dir = facing_right ? -1 : 1;
    int tail_base_x = cx + tail_dir * (len - 1);
    for (int dx = 0; dx < len / 2 + 2; ++dx) {
        int spread = (len / 2 + 2 - dx) / 2 + 1;
        int px = tail_base_x + tail_dir * dx;
        for (int dy = -spread; dy <= spread; ++dy) {
            int py = cy + dy;
            if (px >= 0 && px < RENDER_W && py >= 0 && py < RENDER_H)
                cache_[py * RENDER_W + px] = f.fin_color;
        }
    }

    // Eye.
    int eye_dir = facing_right ? 1 : -1;
    int ex = cx + eye_dir * (len / 2);
    int ey = cy - half_h / 3;
    if (ex >= 0 && ex < RENDER_W && ey >= 0 && ey < RENDER_H)
        cache_[ey * RENDER_W + ex] = 0xFF101010;
}

void FishAquariumEffect::render(uint32_t* buffer, int width, int height) {
    if (!initialized_)
        init_scene();

    // Nearest-neighbour upscale to the real output size, via a LUT so we
    // divide once per column instead of once per pixel per frame. The scene
    // itself is built in update() — once per frame, not once per monitor.
    if (lut_w_ != width) {
        x_lut_.resize(width);
        for (int x = 0; x < width; ++x)
            x_lut_[x] = x * RENDER_W / width;
        lut_w_ = width;
    }

    for (int y = 0; y < height; ++y) {
        int sy = y * RENDER_H / height;
        const uint32_t* src = &cache_[sy * RENDER_W];
        uint32_t* dst = &buffer[y * width];
        for (int x = 0; x < width; ++x)
            dst[x] = src[x_lut_[x]];
    }
}
