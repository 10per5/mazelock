#pragma once

#include <cstdint>

#include "entity.hpp"

class Camera;
class Texture;

class Coin final : public Entity {
public:
    Coin(int x, int y, const Texture* sprite);

    // Entity overrides
    bool try_consume_coin(MazeGenerator& maze, int x, int y) override;
    void render(uint32_t* color_buffer, const float* depth_buffer,
                const Camera& camera,
                int render_w, int render_h, float wall_height) const override;
    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;
    float world_x() const override;
    float world_y() const override;

private:
    int x_, y_;
    bool active_ = true;
    const Texture* sprite_;
};
