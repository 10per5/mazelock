#pragma once

#include "entity.hpp"

class Goal : public Entity {
public:
    Goal(int x, int y, uint32_t color);

    bool occupies(int x, int y) const override;
    uint32_t minimap_color() const override;

private:
    int x_, y_;
    uint32_t color_;
};
