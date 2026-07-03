#pragma once

#include "walk_strategy.hpp"
#include "walker.hpp"
#include "autoplay.hpp"

class AutoWalkStrategy : public WalkStrategy {
public:
    void update(float dt, float& pos_x, float& pos_y,
                float& dir_x, float& dir_y, MazeGenerator& maze,
                float speed) override;

    bool finished() const override { return walker_.finished(); }
    int cell_x() const override { return walker_.cell_x(); }
    int cell_y() const override { return walker_.cell_y(); }
    int steps() const override { return walker_.steps(); }
    bool advancing() const override { return walker_.advancing(); }

    const std::vector<std::pair<int,int>>* path() const override;

    void reset(float pos_x, float pos_y, float dir_x, float dir_y) override;
    void set_consume_check(std::function<bool(int,int)> fn) override;

private:
    Walker walker_;
    AutoplayAI ai_{walker_};
};
