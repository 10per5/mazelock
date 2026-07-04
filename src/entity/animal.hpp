#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "entity.hpp"

class Camera;
class MazeGenerator;
class Texture;

class Animal : public Entity {
public:
    Animal(int x, int y, const Texture* sprite);

    void start_flee(MazeGenerator& maze, float from_x, float from_y);
    void update(float dt);

    void render(uint32_t* color_buffer, const float* depth_buffer,
                const Camera& camera,
                int render_w, int render_h, float wall_height) const;

    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;
    float world_x() const override;
    float world_y() const override;
    bool is_active() const { return active_; }
    bool is_fleeing() const { return fleeing_; }

private:
    int x_, y_;
    bool active_ = true;
    bool fleeing_ = false;
    bool fallback_ = false;
    float flee_pos_x_ = 0.0f, flee_pos_y_ = 0.0f;
    float flee_timer_ = 0.0f;
    float flee_timer_max_ = 0.0f;
    float bob_ = 0.0f;
    const Texture* sprite_;

    std::vector<std::pair<int,int>> flee_path_;
    size_t flee_path_idx_ = 0;
};
