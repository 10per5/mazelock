#pragma once

#include "walk_strategy.hpp"

#include <utility>
#include <vector>

class MazeGenerator;

class GoalSeekerStrategy : public WalkStrategy {
public:
    void update(float dt, float& pos_x, float& pos_y,
                float& dir_x, float& dir_y, MazeGenerator& maze,
                float speed) override;

    const std::vector<std::pair<int,int>>* path() const override;
    bool path_completed() const { return path_done_; }

    void reset(float pos_x, float pos_y, float dir_x, float dir_y) override;
    void set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path);

private:
    std::vector<std::pair<int,int>> path_;
    int path_idx_ = -1;
    int next_dir_ = 1;
    bool path_done_ = false;

    void plan_next(const MazeGenerator& maze);
};
