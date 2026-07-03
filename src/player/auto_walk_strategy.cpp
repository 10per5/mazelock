#include "auto_walk_strategy.hpp"
#include "algorithm/maze.hpp"

void AutoWalkStrategy::update(float dt, float& pos_x, float& pos_y,
                               float& dir_x, float& dir_y, MazeGenerator& maze,
                               float speed) {
    ai_.update(pos_x, pos_y, dir_x, dir_y, maze, speed * dt);
}

const std::vector<std::pair<int,int>>* AutoWalkStrategy::path() const {
    // AutoWalk doesn't expose a path (it's procedural)
    return nullptr;
}

void AutoWalkStrategy::reset(float pos_x, float pos_y, float dir_x, float dir_y) {
    int cx = static_cast<int>(pos_x);
    int cy = static_cast<int>(pos_y);
    int ndir = 1;
    if (dir_x > 0.5f)      ndir = 1;
    else if (dir_x < -0.5f) ndir = 3;
    else if (dir_y < -0.5f) ndir = 0;
    else if (dir_y > 0.5f)  ndir = 2;
    ai_.reset();
    walker_.teleport(cx, cy, ndir);
}

void AutoWalkStrategy::set_consume_check(std::function<bool(int,int)> fn) {
    ai_.set_consume_check(std::move(fn));
}
