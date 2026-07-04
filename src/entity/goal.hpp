#pragma once

#include "entity.hpp"

class Camera;
class Texture;

class Goal : public Entity {
public:
    Goal(int x, int y, uint32_t color, const Texture* sprite);

    void render(uint32_t* color_buffer, const float* depth_buffer,
                const Camera& camera,
                int render_w, int render_h, float wall_height) const;

    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;
    float world_x() const override;
    float world_y() const override;

private:
    int x_, y_;
    uint32_t color_;
    const Texture* sprite_;
};
