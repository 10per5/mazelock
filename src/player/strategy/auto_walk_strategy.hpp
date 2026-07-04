#pragma once

#include "walk_strategy.hpp"

#include <random>
#include <utility>
#include <vector>

class MazeGenerator;

class AutoWalkStrategy : public WalkStrategy {
public:
    void update(float dt, float& pos_x, float& pos_y,
                float& dir_x, float& dir_y, MazeGenerator& maze,
                float speed) override;

    void reset(float pos_x, float pos_y, float dir_x, float dir_y) override;

    void set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path);
    bool has_path() const { return path_idx_ >= 0; }
    void clear_path();
    void request_reverse();

private:
    void plan_next_step(const MazeGenerator& maze);
    int do_reverse();
    bool on_step_complete(const MazeGenerator& maze);

    std::mt19937 rng_{std::random_device{}()};

    int pause_ = 0;
    bool reverse_requested_ = false;

    // Path following state
    std::vector<std::pair<int,int>> path_;
    int path_idx_ = -1;

    static constexpr int PAUSE_FRAMES = 30;
};
