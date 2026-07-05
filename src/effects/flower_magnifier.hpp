#pragma once

#include "effect.hpp"

#include <cstdint>
#include <vector>

class TextureManager;

// Draws a small flower "far away" in the centre of frame, then slowly
// magnifies it until it fills a centred 16:9 viewport. The viewport is
// letterboxed/pillarboxed inside whatever the real output aspect ratio is,
// so the (square) flower art is scaled up cleanly rather than stretched
// wide to fill an ultrawide or unusual-aspect display.
class FlowerMagnifierEffect : public Effect {
public:
    explicit FlowerMagnifierEffect(const TextureManager& textures);
    ~FlowerMagnifierEffect() override = default;

    void update(float dt) override;
    void render(uint32_t* buffer, int width, int height) override;
    const char* name() const override { return "Flower Magnifier"; }

private:
    enum class Phase : uint8_t { FAR, ZOOM_IN, HOLD, ZOOM_OUT };

    void pick_next_flower();
    void rebuild_cache();

    const TextureManager& textures_;
    int flower_index_ = 0;

    Phase phase_ = Phase::FAR;
    float phase_time_ = 0.0f; // seconds elapsed in current phase
    float bob_phase_ = 0.0f;
    float time_ = 0.0f;       // free-running clock, drives the color wash

    // Internal 16:9 composition buffer for the flower + its background.
    // Nearest-neighbour upscaled into a centred viewport in render().
    static constexpr int CACHE_W = 320;
    static constexpr int CACHE_H = 180;
    std::vector<uint32_t> cache_;

    static constexpr float FAR_HOLD_TIME  = 3.0f;
    static constexpr float ZOOM_TIME      = 4.0f;
    static constexpr float FULL_HOLD_TIME = 5.0f;

    // Upscale LUTs, rebuilt only when the destination viewport size changes.
    std::vector<int> x_lut_;
    std::vector<int> y_lut_;
    int lut_w_ = -1;
    int lut_h_ = -1;
};
