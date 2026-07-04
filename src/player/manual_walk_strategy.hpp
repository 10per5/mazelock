#pragma once

#include "walk_strategy.hpp"
#include "walker.hpp"
#include "autoplay.hpp"

class MazeGenerator;

class ManualWalkStrategy : public WalkStrategy {
public:
    void update(float dt, float& pos_x, float& pos_y,
                float& dir_x, float& dir_y, MazeGenerator& maze,
                float speed) override;

    bool finished() const override { return walker_.finished(); }
    int cell_x() const override { return walker_.cell_x(); }
    int cell_y() const override { return walker_.cell_y(); }
    int steps() const override { return walker_.steps(); }
    bool advancing() const override { return walker_.advancing(); }

    void reset(float pos_x, float pos_y, float dir_x, float dir_y) override;
    void set_consume_check(std::function<bool(int,int)> fn) override;
    void set_god_mode(bool g) { ai_.set_god_mode(g); }

    // Direct manual commands (called by Player)
    bool move_forward(const MazeGenerator& maze);
    bool move_back(const MazeGenerator& maze);
    void turn_left();
    void turn_right();

private:
    Walker walker_;
    AutoplayAI ai_{walker_};
};
