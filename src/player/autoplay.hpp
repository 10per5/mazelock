#pragma once

#include <functional>
#include <random>
#include <utility>
#include <vector>

#include "walker.hpp"

class MazeGenerator;

class AutoplayAI {
    Walker& walker_;
    std::mt19937 rng_{std::random_device{}()};
    std::function<bool(int,int)> consume_check_;

    int pause_ = 0;
    bool reverse_requested_ = false;

    // Path following state
    std::vector<std::pair<int,int>> path_;
    int path_idx_ = -1;

    // When true, manual control is active and AI planning is paused
    bool manual_mode_ = false;

    static constexpr int PAUSE_FRAMES = 30;

    bool disable_animal_reverse_ = false;
    bool god_mode_ = false;

    void plan_next_step(const MazeGenerator& maze);
    int do_reverse();  // returns the reversed direction
    bool on_step_complete(const MazeGenerator& maze);

public:
    explicit AutoplayAI(Walker& w) : walker_(w) {}

    bool finished() const { return walker_.finished(); }
    int cell_x() const { return walker_.cell_x(); }
    int cell_y() const { return walker_.cell_y(); }
    int steps() const { return walker_.steps(); }

    void set_consume_check(std::function<bool(int,int)> fn) { consume_check_ = std::move(fn); }
    void set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path);
    bool has_path() const { return path_idx_ >= 0; }
    void clear_path();
    void request_reverse();
    void reset();
    void set_manual_mode(bool m) { manual_mode_ = m; }
    void set_god_mode(bool g) { god_mode_ = g; }

    // Manual control — grid-aligned, turn-based movement
    bool manual_forward(const MazeGenerator& maze);
    bool manual_back(const MazeGenerator& maze);
    bool manual_turn_left();
    bool manual_turn_right();

    void update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                const MazeGenerator& maze, float speed);
};
