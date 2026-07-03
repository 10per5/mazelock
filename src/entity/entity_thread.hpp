#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ui/camera.hpp"

class Entity;
class MazeGenerator;
class Texture;
class TextureManager;
class Coin;

class EntityThread {
public:
    EntityThread();
    ~EntityThread();

    void set_sprites(const TextureManager& tm);

    void init(MazeGenerator& maze);
    void update(float dt, MazeGenerator& maze, int ai_cell_x, int ai_cell_y,
                float pos_x, float pos_y, float dir_x, float dir_y);

    void render_sprites(uint32_t* color_buffer, const float* depth_buffer,
                         const Camera& camera,
                         int render_w, int render_h, float wall_height) const;

    bool consume_animal_at(int x, int y, MazeGenerator& maze, float player_x, float player_y);
    bool consume_coin_at(int x, int y, MazeGenerator& maze);

    uint32_t screen_overlay_color() const;
    float screen_overlay_alpha() const;

    const std::vector<Entity*>& entities() const { return entities_; }
    int score() const { return score_; }
    void reset();

    Camera& camera() { return camera_; }
    const Camera& camera() const { return camera_; }

private:
    std::vector<std::unique_ptr<Entity>> owned_;
    std::vector<Entity*> entities_;
    const Texture* rat_sprite_ = nullptr;
    const Texture* coin_sprite_ = nullptr;
    float animal_flash_alpha_ = 0.0f;
    int score_ = 0;
    Camera camera_;
};
