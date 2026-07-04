#pragma once

#include <cstdint>

class Camera;
class MazeGenerator;

class Entity {
public:
    virtual ~Entity() = default;

    // Queries
    virtual bool occupies(int x, int y) const = 0;
    virtual uint32_t minimap_color() const = 0;
    virtual float world_x() const = 0;
    virtual float world_y() const = 0;

    // Per-frame update (no-op for static entities)
    virtual void update(float dt) {}

    // Player interaction — each subtype decides if it's consumed
    virtual bool try_consume_animal(MazeGenerator& maze, float player_x, float player_y, int x, int y) { return false; }
    virtual bool try_consume_coin(MazeGenerator& maze, int x, int y) { return false; }

    // Render
    virtual void render(uint32_t* color_buffer, const float* depth_buffer,
                        const Camera& camera,
                        int render_w, int render_h, float wall_height) const = 0;
};
