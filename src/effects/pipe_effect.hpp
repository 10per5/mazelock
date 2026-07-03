#pragma once

#include "effect.hpp"

#include <cstdint>
#include <vector>

class PipeEffect : public Effect {
public:
    PipeEffect();
    ~PipeEffect() override = default;

    void update(float dt) override;
    void render(uint32_t* buffer, int width, int height) override;
    const char* name() const override { return "3D Pipes"; }

    void resize(int width, int height);

private:
    enum class Dir : uint8_t { N = 0, E = 1, S = 2, W = 3 };
    static constexpr int DIRS = 4;

    struct Segment {
        int gx, gy;
        Dir dir;
        uint32_t color;
    };

    static Dir opposite(Dir d);
    static bool is_horiz(Dir d);
    static int  grid_x(Dir d);
    static int  grid_y(Dir d);

    void grow();
    void seed_chain();
    void redraw_cell(int gx, int gy);
    bool occupied(int gx, int gy, Dir d) const;

    int grid_w_ = 0;
    int grid_h_ = 0;
    int cell_w_ = 0;
    int cell_h_ = 0;

    // occupancy: 4 bits per cell (N=1, E=2, S=4, W=8)
    std::vector<uint8_t> occ_;

    std::vector<Segment> segments_;
    uint32_t current_color_ = 0xFFFFFFFF;

    // Pipe tip — where the next segment grows from
    int tip_gx_ = 0;
    int tip_gy_ = 0;
    Dir tip_dir_ = Dir::E;

    float timer_ = 0.0f;
    float interval_ = 0.05f;

    static constexpr int RENDER_W = 320;
    static constexpr int RENDER_H = 200;

    bool dirty_ = true;

    enum Phase : uint8_t { DRAW, UNDRAW, IDLE };
    Phase phase_ = DRAW;
    int erased_count_ = 0;
    std::vector<uint8_t> erased_;
    int undraw_gx_ = 0;
    int undraw_gy_ = 0;
    Dir undraw_dir_ = Dir::E;
    std::vector<Segment> undraw_segments_;
    float idle_timer_ = 0.0f;

    int cache_w_ = 0;
    int cache_h_ = 0;
    std::vector<uint32_t> cache_;

    void draw_segment(uint32_t* buf, int bw, int bh, const Segment& seg);

    // Colour palette (W95-style bright colours)
    static constexpr uint32_t PALETTE[8] = {
        0xFF0088FF, // blue
        0xFFFF4400, // orange
        0xFF00CC44, // green
        0xFFFF00FF, // magenta
        0xFF44CCFF, // cyan
        0xFFFFFF00, // yellow
        0xFFFF4488, // pink
        0xFF8844FF, // purple
    };
};
