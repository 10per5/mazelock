#pragma once

#include "effect.hpp"

#include <cstdint>
#include <vector>

// Slow-swimming fish tank screensaver. Deliberately cheap: everything is
// simulated at a fixed low internal resolution (RENDER_W x RENDER_H) and
// nearest-neighbour upscaled to the actual monitor size, same approach as
// PipeEffect. No per-pixel trig beyond a handful of fish/kelp/bubbles.
class FishAquariumEffect : public Effect {
public:
    FishAquariumEffect();
    ~FishAquariumEffect() override = default;

    void update(float dt) override;
    void render(uint32_t* buffer, int width, int height) override;
    const char* name() const override { return "Fish Aquarium"; }

private:
    struct Fish {
        float x = 0.0f;      // px, cache space
        float base_y = 0.0f; // centreline the fish bobs around
        float speed = 0.0f;  // px/sec, sign gives direction
        float bob_phase = 0.0f;
        float bob_freq = 0.0f;
        float bob_amp = 0.0f;
        int   len = 0;       // half-length in px
        uint32_t body_color = 0;
        uint32_t fin_color = 0;
    };

    struct Kelp {
        int   base_x = 0;
        int   height = 0;
        float sway_phase = 0.0f;
    };

    struct Bubble {
        float x = 0.0f;
        float y = 0.0f;
        float speed = 0.0f;
        int   radius = 1;
    };

    struct Pebble {
        int x = 0;
        int y = 0;
        int radius = 1;
        uint32_t color = 0;
    };

    void init_scene();
    void respawn_fish(Fish& f, bool at_edge);
    void respawn_bubble(Bubble& b);
    void draw_fish(const Fish& f);
    void draw_kelp(const Kelp& k);

    static constexpr int RENDER_W = 320;
    static constexpr int RENDER_H = 200;

    std::vector<uint32_t> cache_;
    std::vector<Fish> fish_;
    std::vector<Kelp> kelp_;
    std::vector<Bubble> bubbles_;
    std::vector<Pebble> pebbles_;

    float time_ = 0.0f;
    bool initialized_ = false;

    // Nearest-neighbour upscale LUT — built once per output size instead of
    // dividing on every pixel every frame.
    std::vector<int> x_lut_;
    int lut_w_ = -1;
};
