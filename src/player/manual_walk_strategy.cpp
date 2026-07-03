#include "manual_walk_strategy.hpp"
#include "algorithm/maze.hpp"

void ManualWalkStrategy::update(float dt, float& pos_x, float& pos_y,
                                 float& dir_x, float& dir_y, MazeGenerator& maze,
                                 float speed) {
    ai_.update(pos_x, pos_y, dir_x, dir_y, maze, speed * dt);
}

void ManualWalkStrategy::reset(float pos_x, float pos_y, float dir_x, float dir_y) {
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

void ManualWalkStrategy::set_consume_check(std::function<bool(int,int)> fn) {
    ai_.set_consume_check(std::move(fn));
}

bool ManualWalkStrategy::move_forward(const MazeGenerator& maze) {
    ai_.set_manual_mode(true);
    return ai_.manual_forward(maze);
}

bool ManualWalkStrategy::move_back(const MazeGenerator& maze) {
    ai_.set_manual_mode(true);
    return ai_.manual_back(maze);
}

void ManualWalkStrategy::turn_left() {
    ai_.set_manual_mode(true);
    ai_.manual_turn_left();
}

void ManualWalkStrategy::turn_right() {
    ai_.set_manual_mode(true);
    ai_.manual_turn_right();
}
