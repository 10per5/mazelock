#pragma once

#include <cstdint>

class Entity {
public:
    virtual ~Entity() = default;
    virtual bool occupies(int x, int y) const = 0;
    virtual uint32_t minimap_color() const = 0;
};
