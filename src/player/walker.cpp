#include "walker.hpp"
#include "../cfg/constants.hpp"
#include "game/singleton.hpp"
#include "algorithm/maze.hpp"

#include <cmath>

static const char* dir_name(int d) {
    static const char* names[] = {"N", "E", "S", "W"};
    return names[d];
}

// -- turn-around -----------------------------------------------------------

void Walker::start_turnaround(const MazeGenerator& maze) {
    int cur = direction_;
    // Check order: 180° (opposite), +1 (right/clockwise), +3 (left), +0 (forward)
    static constexpr int order[] = {2, 1, 3, 0};
    int best = cur;
    for (int offset : order) {
        int dir = (cur + offset) % 4;
        if (!maze.is_wall(cell_x_, cell_y_, dir)) {
            best = dir;
            break;
        }
    }
    turnaround_target_dir_ = best;
    g_logger->debug("[WALKER] start_turnaround: cur=%s target=%s",
               dir_name(cur), dir_name(best));
}

bool Walker::advance_turnaround() {
    if (turnaround_target_dir_ < 0)
        return true;
    if (direction_ == turnaround_target_dir_) {
        turnaround_target_dir_ = -1;
        return true;
    }
    plan_turn((direction_ + 3) % 4);
    return false;
}

void Walker::execute_move(int dir) {
    start_x_ = static_cast<float>(cell_x_) + 0.5f;
    start_y_ = static_cast<float>(cell_y_) + 0.5f;
    start_angle_ = static_cast<float>(dir) * PI_2 - PI_2;
    end_x_ = static_cast<float>(cell_x_ + dx[dir]) + 0.5f;
    end_y_ = static_cast<float>(cell_y_ + dy[dir]) + 0.5f;
    end_angle_ = start_angle_;
    next_cell_x_ = cell_x_ + dx[dir];
    next_cell_y_ = cell_y_ + dy[dir];
    next_dir_ = dir;
    turn_divider_ = 1.0f;
    turning_ = false;
    stepping_ = true;
}

void Walker::execute_turn(int dir) {
    start_x_ = static_cast<float>(cell_x_) + 0.5f;
    start_y_ = static_cast<float>(cell_y_) + 0.5f;
    start_angle_ = static_cast<float>(direction_) * PI_2 - PI_2;
    end_x_ = start_x_;
    end_y_ = start_y_;
    float raw_end = static_cast<float>(dir) * PI_2 - PI_2;
    float delta = raw_end - start_angle_;
    if (delta > PI) delta -= 2.0f * PI;
    if (delta < -PI) delta += 2.0f * PI;
    end_angle_ = start_angle_ + delta;
    next_cell_x_ = cell_x_;
    next_cell_y_ = cell_y_;
    next_dir_ = dir;
    turning_ = true;
    turn_divider_ = (std::abs(delta) > PI_2 + 0.01f) ? 3.0f : 2.0f;
    stepping_ = true;
}

void Walker::flush_pending() {
    if (pending_dir_ < 0) return;
    if (!pending_turn_ && !god_mode_ && maze_) {
        if (maze_->is_wall(cell_x_, cell_y_, pending_dir_)) {
            start_bump(pending_dir_);
            pending_dir_ = -1;
            return;
        }
        // Animal in target cell — trigger flee and cancel the move
        int nx = cell_x_ + dx[pending_dir_];
        int ny = cell_y_ + dy[pending_dir_];
        if (consume_check_ && consume_check_(nx, ny)) {
            start_bump(pending_dir_);
            if (on_animal_) on_animal_();
            pending_dir_ = -1;
            return;
        }
    }
    if (pending_turn_)
        execute_turn(pending_dir_);
    else
        execute_move(pending_dir_);
    pending_dir_ = -1;
    step_ = 0.0f;
}

bool Walker::plan_move(int dir) {
    // Wall check for all callers
    if (!god_mode_ && maze_ && maze_->is_wall(cell_x_, cell_y_, dir)) {
        start_bump(dir);
        return false;
    }

    // Animal in target cell — trigger flee and let the strategy handle reversing
    if (!god_mode_ && maze_ && consume_check_) {
        int nx = cell_x_ + dx[dir];
        int ny = cell_y_ + dy[dir];
        if (consume_check_(nx, ny)) {
            start_bump(dir);
            if (on_animal_) on_animal_();
            return true;
        }
    }
    if (stepping_ && !finished_) {
        pending_dir_ = dir;
        pending_turn_ = false;
        return true;
    }
    execute_move(dir);
    return false;
}

bool Walker::plan_turn(int dir) {
    if (stepping_ && !finished_) {
        pending_dir_ = dir;
        pending_turn_ = true;
        return true;
    }
    execute_turn(dir);
    return false;
}

void Walker::teleport(int x, int y, int dir) {
    cell_x_ = x;
    cell_y_ = y;
    next_cell_x_ = x;
    next_cell_y_ = y;
    direction_ = dir;
    next_dir_ = dir;
    turning_ = false;
    turn_divider_ = 1.0f;
    step_ = 1.0f;
    finished_ = false;
    stepping_ = false;
    pending_dir_ = -1;
    turnaround_target_dir_ = -1;
    consume_pause_ = 0;
    bump_frames_ = 0;
    start_x_ = static_cast<float>(x) + 0.5f;
    start_y_ = static_cast<float>(y) + 0.5f;
    start_angle_ = static_cast<float>(dir) * PI_2 - PI_2;
    end_x_ = start_x_;
    end_y_ = start_y_;
    end_angle_ = start_angle_;
}

void Walker::hold_position() {
    start_x_ = static_cast<float>(cell_x_) + 0.5f;
    start_y_ = static_cast<float>(cell_y_) + 0.5f;
    start_angle_ = static_cast<float>(direction_) * PI_2 - PI_2;
    end_x_ = start_x_;
    end_y_ = start_y_;
    end_angle_ = start_angle_;
    next_cell_x_ = cell_x_;
    next_cell_y_ = cell_y_;
    next_dir_ = direction_;
    turning_ = false;
    turn_divider_ = 1e6f;
    stepping_ = false;
}

static constexpr int BUMP_FRAMES = 10;

void Walker::start_bump(int dir) {
    if (bump_frames_ > 0) return;  // already playing
    bump_dir_ = dir;
    bump_frames_ = BUMP_FRAMES;
}

void Walker::reset() {
    cell_x_ = 0;
    cell_y_ = 0;
    direction_ = 1;
    next_cell_x_ = 0;
    next_cell_y_ = 0;
    next_dir_ = 1;
    step_ = 1.0f;
    turning_ = false;
    turn_divider_ = 1.0f;
    finished_ = false;
    steps_ = 0;
    stepping_ = false;
    pending_dir_ = -1;
    turnaround_target_dir_ = -1;
    consume_pause_ = 0;
    bump_frames_ = 0;
    start_x_ = end_x_ = 0.5f;
    start_y_ = end_y_ = 0.5f;
    start_angle_ = end_angle_ = 0.0f;
}

void Walker::apply_bump(float& pos_x, float& pos_y) {
    if (bump_frames_ <= 0) return;
    float bt = static_cast<float>(BUMP_FRAMES - bump_frames_) / BUMP_FRAMES;
    float offset = 0.08f * std::sin(bt * PI);
    pos_x += dx[bump_dir_] * offset;
    pos_y += dy[bump_dir_] * offset;
    --bump_frames_;
}

void Walker::snap_to_cell(float& pos_x, float& pos_y, float& dir_x, float& dir_y) {
    pos_x = static_cast<float>(cell_x_) + 0.5f;
    pos_y = static_cast<float>(cell_y_) + 0.5f;
    float angle = static_cast<float>(direction_) * PI_2 - PI_2;
    dir_x = std::cos(angle);
    dir_y = std::sin(angle);
}

void Walker::update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                    float speed, const StepCallback& plan_next) {
    if (finished_) {
        apply_bump(pos_x, pos_y);
        return;
    }

    // Consume pause: hold position for a few frames so the player can see
    // the animal flee animation before the strategy decides the next action.
    if (consume_pause_ > 0) {
        if (--consume_pause_ != 0)
            hold_position();
        snap_to_cell(pos_x, pos_y, dir_x, dir_y);
        apply_bump(pos_x, pos_y);
        return;
    }

    // In manual mode, freeze the walker when idle so it doesn't drift
    if (freeze_when_idle_ && !stepping_)
        hold_position();

    step_ += speed / turn_divider_;

    while (step_ >= 1.0f) {
        step_ -= 1.0f;
        cell_x_ = next_cell_x_;
        cell_y_ = next_cell_y_;
        direction_ = next_dir_;

        stepping_ = false;
        flush_pending();
        ++steps_;

        // Consume check — pause so the player sees the bump + flee before reversing.
        bool consumed = consume_check_ && consume_check_(cell_x_, cell_y_);
        if (consumed) {
            consume_pause_ = CONSUME_PAUSE_FRAMES;
            if (on_animal_) on_animal_();
            g_logger->debug("[WALKER] consumed at (%d,%d) — pause before reverse",
                       cell_x_, cell_y_);
            start_bump(direction_);
            snap_to_cell(pos_x, pos_y, dir_x, dir_y);
            apply_bump(pos_x, pos_y);
            return;
        }

        // Finish detection
        if (maze_ && maze_->is_finish(cell_x_, cell_y_)) {
            finished_ = true;
            g_logger->debug("[WALKER] SOLVED in %d steps!", steps_);
            snap_to_cell(pos_x, pos_y, dir_x, dir_y);
            apply_bump(pos_x, pos_y);
            return;
        }

        // Strategy callback — skip if we already reversed from consume
        if (!consumed && plan_next && plan_next()) {
            snap_to_cell(pos_x, pos_y, dir_x, dir_y);
            apply_bump(pos_x, pos_y);
            return;
        }
    }

    {
        float t = step_;
        pos_x = start_x_ + (end_x_ - start_x_) * t;
        pos_y = start_y_ + (end_y_ - start_y_) * t;
        float angle = start_angle_ + (end_angle_ - start_angle_) * t;
        dir_x = std::cos(angle);
        dir_y = std::sin(angle);
    }

    apply_bump(pos_x, pos_y);
}

bool Walker::manual_forward() {
    int dir = turning_ ? next_dir_ : direction_;
    if (!god_mode_ && maze_ && maze_->is_wall(cell_x_, cell_y_, dir)) {
        start_bump(dir);
        return false;
    }

    // Animal in the target cell — trigger flee and bump in place
    if (!god_mode_ && maze_ && consume_check_) {
        int nx = cell_x_ + dx[dir];
        int ny = cell_y_ + dy[dir];
        if (consume_check_(nx, ny)) {
            start_bump(dir);
            return true;
        }
    }

    if (!plan_move(dir))
        restart_step();
    g_logger->debug("[MANUAL] forward (%d,%d) %s",
               cell_x_, cell_y_, dir_name(dir));
    return true;
}

bool Walker::manual_back() {
    int back_dir = turning_ ? (next_dir_ + 2) % 4 : (direction_ + 2) % 4;
    if (!god_mode_ && maze_ && maze_->is_wall(cell_x_, cell_y_, back_dir)) {
        start_bump(back_dir);
        return false;
    }

    // Animal in the target cell — trigger flee and bump in place
    if (!god_mode_ && maze_ && consume_check_) {
        int nx = cell_x_ + dx[back_dir];
        int ny = cell_y_ + dy[back_dir];
        if (consume_check_(nx, ny)) {
            start_bump(back_dir);
            return true;
        }
    }

    if (!plan_move(back_dir))
        restart_step();
    g_logger->debug("[MANUAL] back (%d,%d) now %s",
               cell_x_, cell_y_, dir_name(back_dir));
    return true;
}

void Walker::manual_turn_left() {
    int new_dir = (direction_ + 3) % 4;
    if (!plan_turn(new_dir))
        restart_step();
    g_logger->debug("[MANUAL] turn left (%d,%d) now %s",
               cell_x_, cell_y_, dir_name(new_dir));
}

void Walker::manual_turn_right() {
    int new_dir = (direction_ + 1) % 4;
    if (!plan_turn(new_dir))
        restart_step();
    g_logger->debug("[MANUAL] turn right (%d,%d) now %s",
               cell_x_, cell_y_, dir_name(new_dir));
}
