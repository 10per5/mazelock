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

private:
    void plan_next_step(const MazeGenerator& maze);
    bool on_step_complete(const MazeGenerator& maze);

    std::mt19937 rng_{std::random_device{}()};
    std::uniform_int_distribution<int> random_explore_dist_{0, 19};

    int pause_ = 0;
    int reverse_pause_ = 0;
    bool reversing_ = false;
    int reverse_phase_ = 0;   // number of 90° turns completed so far
    int reverse_target_ = 2;  // how many 90° turns to attempt
    int reverse_steps_ = -1;  // remaining forward steps after reverse turn; -1 = inactive
    bool on_animal_setup_ = false;

    // Path following state
    std::vector<std::pair<int,int>> path_;
    int path_idx_ = -1;

    static constexpr int PAUSE_FRAMES = 30;
};
