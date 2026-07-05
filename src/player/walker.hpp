#pragma once

#include "../cfg/constants.hpp"

#include <functional>

class MazeGenerator;

class Walker {
public:
    using StepCallback = std::function<bool()>;  // return true = skip remaining steps this frame

    // Accessors
    int cell_x() const { return cell_x_; }
    int cell_y() const { return cell_y_; }
    int direction() const { return direction_; }
    int target_direction() const { return next_dir_; }
    bool finished() const { return finished_; }
    bool turning() const { return turning_; }
    int steps() const { return steps_; }

    void set_maze(MazeGenerator* m) { maze_ = m; }

    // Plan a forward step in the given direction.
    // Returns true if the action was queued (will run after current step finishes).
    bool plan_move(int dir);

    // Plan a turn-in-place toward the given direction.
    // Returns true if the action was queued (will run after current step finishes).
    bool plan_turn(int dir);

    // Teleport to a cell and direction (no interpolation).
    void teleport(int x, int y, int dir);

    void set_finished(bool f) { finished_ = f; }
    void restart_step() { step_ = 0.0f; }
    void hold_position();
    bool advancing() const { return stepping_; }
    void reset();

    bool has_pending() const { return pending_dir_ >= 0; }
    bool consuming() const { return consume_pause_ > 0; }

    // Consume callback — called after each step; if set and returns true, walker auto-reverses
    using ConsumeCheck = std::function<bool(int,int)>;
    void set_consume_check(ConsumeCheck fn) { consume_check_ = std::move(fn); }

    // Called when an animal is detected in the target cell (before a move)
    using OnAnimal = std::function<void()>;
    void set_on_animal(OnAnimal fn) { on_animal_ = std::move(fn); }

    // God mode — walk through walls
    void set_god_mode(bool g) { god_mode_ = g; }
    bool god_mode() const { return god_mode_; }

    // Idle freeze — when true, walker auto-holds position when not advancing (manual mode)
    void set_freeze_when_idle(bool f) { freeze_when_idle_ = f; }
    bool freeze_when_idle() const { return freeze_when_idle_; }

    // Manual control — grid-aligned, turn-based movement
    bool manual_forward();
    bool manual_back();
    void manual_turn_left();
    void manual_turn_right();

    // "Attempt to walk in" bump animation (brief step-into-wall then back)
    void start_bump(int dir);
    bool bumping() const { return bump_frames_ > 0; }

    // Turn-around sequence: find best open direction (180° → +1 → +3 → 0°)
    // and execute phased 90° left turns. Call start_turnaround once, then
    // call advance_turnaround() each logical frame (after step completes).
    void start_turnaround(const MazeGenerator& maze);
    bool advance_turnaround();
    bool turnaround_active() const { return turnaround_target_dir_ >= 0; }

    // Advance interpolation and call on_complete for each completed step.
    // plan_next is called after each step to let the strategy queue the next action.
    // Returns true if plan_next returns true (strategy signals to bail).
    void update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                float speed, const StepCallback& plan_next = nullptr);

private:
    void execute_move(int dir);
    void execute_turn(int dir);
    void flush_pending();
    void snap_to_cell(float& pos_x, float& pos_y, float& dir_x, float& dir_y);
    void apply_bump(float& pos_x, float& pos_y);

    int cell_x_ = 0, cell_y_ = 0;
    int direction_ = 1;
    float step_ = 1.0f;
    float start_x_ = 0.0f, start_y_ = 0.0f, start_angle_ = 0.0f;
    float end_x_ = 0.0f, end_y_ = 0.0f, end_angle_ = 0.0f;
    int next_cell_x_ = 0, next_cell_y_ = 0, next_dir_ = 1;
    bool turning_ = false;
    float turn_divider_ = 1.0f;
    bool finished_ = false;
    int steps_ = 0;

    int pending_dir_ = -1;
    bool pending_turn_ = false;
    bool stepping_ = false;

    int bump_frames_ = 0;
    int bump_dir_ = 0;

    MazeGenerator* maze_ = nullptr;
    bool god_mode_ = false;
    bool freeze_when_idle_ = false;
    int consume_pause_ = 0;
    ConsumeCheck consume_check_;
    OnAnimal on_animal_;
    int turnaround_target_dir_ = -1;

public:
    static constexpr int CONSUME_PAUSE_FRAMES = 30;
    static constexpr int dx[4] = { 0, 1, 0, -1 };
    static constexpr int dy[4] = {-1, 0, 1,  0 };
};
