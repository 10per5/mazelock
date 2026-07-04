#pragma once

#include "strategy/walk_strategy.hpp"

#include <memory>
#include <utility>
#include <vector>

class MazeGenerator;
class EntityThread;
class AutoWalkStrategy;
class GoalSeekerStrategy;

class Player {
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void init(MazeGenerator& maze, EntityThread& entities);

    void update(float dt);

    // Actions — call from key handlers in main
    bool move_forward();
    bool move_back();
    void turn_left();
    void turn_right();

    // Mode switching
    void enable_autowalk();
    void pathfind_to_finish();
    void set_god_mode(bool g);

    // Queries
    bool finished() const { return current_->finished(); }
    int cell_x() const { return current_->cell_x(); }
    int cell_y() const { return current_->cell_y(); }
    int steps() const { return current_->steps(); }
    bool advancing() const { return current_->advancing(); }

    float pos_x() const { return pos_x_; }
    float pos_y() const { return pos_y_; }
    float dir_x() const { return dir_x_; }
    float dir_y() const { return dir_y_; }

    const std::vector<std::pair<int,int>>* path() const { return current_->path(); }

    void reset();

    // Held-direction for auto-repeat (set each frame from main)
    void set_held_dir(int dir) { held_dir_ = dir; }
    void set_speed_mult(float m) { speed_mult_ = m; }

private:
    void switch_to(WalkStrategy* s);
    void enable_manual();

    std::unique_ptr<AutoWalkStrategy> autowalk_;
    std::unique_ptr<GoalSeekerStrategy> seeker_;
    WalkStrategy* current_ = nullptr;

    float pos_x_ = 0.5f, pos_y_ = 0.5f;
    float dir_x_ = 1.0f, dir_y_ = 0.0f;

    MazeGenerator* maze_ = nullptr;
    EntityThread* entities_ = nullptr;

    float ai_speed_ = 2.0f;
    float speed_mult_ = 1.0f;

    int prev_cx_ = -1, prev_cy_ = -1;
    int held_dir_ = 0; // 0=none, 1=fwd, 2=back, 3=left, 4=right

    Walker::ConsumeCheck consume_fn_;
};
