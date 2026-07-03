#pragma once

#include "walk_strategy.hpp"
#include "walker.hpp"

#include <utility>
#include <vector>

class MazeGenerator;

class GoalSeekerStrategy : public WalkStrategy {
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
    bool path_completed() const { return path_done_; }

    void reset(float pos_x, float pos_y, float dir_x, float dir_y) override;
    void set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path);

private:
    Walker walker_;
    std::vector<std::pair<int,int>> path_;
    int path_idx_ = -1;
    int next_dir_ = 1;
    bool path_done_ = false;

    void plan_next(const MazeGenerator& maze);
};
