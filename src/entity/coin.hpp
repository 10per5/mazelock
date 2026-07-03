#pragma once

#include <cstdint>

#include "entity.hpp"

class Camera;
class Texture;

class Coin : public Entity {
public:
    Coin(int x, int y, const Texture* sprite);

    void remove();

    void render(uint32_t* color_buffer, const float* depth_buffer,
                const Camera& camera,
                int render_w, int render_h, float wall_height) const;

    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;
    bool is_active() const { return active_; }

private:
    int x_, y_;
    bool active_ = true;
    const Texture* sprite_;
};
