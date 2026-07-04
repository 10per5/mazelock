#pragma once

#include <cstdint>

class Entity {
public:
    virtual ~Entity() = default;
    virtual bool occupies(int x, int y) const = 0;
    virtual uint32_t minimap_color() const = 0;
    virtual float world_x() const = 0;
    virtual float world_y() const = 0;
};
