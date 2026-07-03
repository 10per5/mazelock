#include "goal.hpp"

Goal::Goal(int x, int y, uint32_t color) : x_(x), y_(y), color_(color) {}

bool Goal::occupies(int x, int y) const {
    return x == x_ && y == y_;
}

uint32_t Goal::minimap_color() const {
    return color_;
}
