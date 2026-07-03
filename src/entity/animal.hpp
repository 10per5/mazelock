#pragma once

#include <cstdint>

#include "entity.hpp"

class Camera;
class Texture;

class Animal : public Entity {
public:
    Animal(int x, int y, const Texture* sprite);

    void start_flee(float from_x, float from_y);
    void update(float dt);

    void render(uint32_t* color_buffer, const float* depth_buffer,
                const Camera& camera,
                int render_w, int render_h, float wall_height) const;

    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;
    bool is_active() const { return active_; }
    bool is_fleeing() const { return fleeing_; }

private:
    int x_, y_;
    bool active_ = true;
    bool fleeing_ = false;
    float flee_pos_x_, flee_pos_y_;
    float flee_vx_ = 0.0f, flee_vy_ = 0.0f;
    float flee_timer_ = 0.0f;
    float bob_ = 0.0f;
    const Texture* sprite_;
};
