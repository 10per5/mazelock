#pragma once

#include <functional>

class Walker {
public:
    using Callback = std::function<bool()>;
    using CollisionCheck = std::function<bool(int, int, int)>;  // x, y, dir -> allowed

    // Accessors
    int cell_x() const { return cell_x_; }
    int cell_y() const { return cell_y_; }
    int direction() const { return direction_; }
    bool finished() const { return finished_; }
    bool turning() const { return turning_; }
    int steps() const { return steps_; }

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

    void set_collision_check(CollisionCheck fn) { collision_check_ = std::move(fn); }

    // Advance interpolation and call on_complete for each completed step.
    // If on_complete returns true, the walker returns immediately with cell-centre position.
    void update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                float speed, const Callback& on_complete);

private:
    void execute_move(int dir);
    void execute_turn(int dir);
    void flush_pending();

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

    CollisionCheck collision_check_;

    static constexpr float PI_2 = 1.57079632679f;
    static constexpr int dx[4] = { 0, 1, 0, -1 };
    static constexpr int dy[4] = {-1, 0, 1,  0 };
};
