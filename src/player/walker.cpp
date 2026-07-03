#include "walker.hpp"

#include <cmath>

static constexpr float PI = 3.14159265358979f;

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
    if (!pending_turn_ && collision_check_ && !collision_check_(cell_x_, cell_y_, pending_dir_)) {
        pending_dir_ = -1;
        return;
    }
    if (pending_turn_)
        execute_turn(pending_dir_);
    else
        execute_move(pending_dir_);
    pending_dir_ = -1;
    step_ = 0.0f;
}

bool Walker::plan_move(int dir) {
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
    start_x_ = end_x_ = 0.5f;
    start_y_ = end_y_ = 0.5f;
    start_angle_ = end_angle_ = 0.0f;
}

void Walker::update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                    float speed, const Callback& on_complete) {
    if (finished_) return;

    step_ += speed / turn_divider_;

    while (step_ >= 1.0f) {
        step_ -= 1.0f;
        cell_x_ = next_cell_x_;
        cell_y_ = next_cell_y_;
        direction_ = next_dir_;

        stepping_ = false;
        flush_pending();

        if (on_complete) {
            ++steps_;
            if (on_complete()) {
                pos_x = static_cast<float>(cell_x_) + 0.5f;
                pos_y = static_cast<float>(cell_y_) + 0.5f;
                float angle = static_cast<float>(direction_) * PI_2 - PI_2;
                dir_x = std::cos(angle);
                dir_y = std::sin(angle);
                return;
            }
        }
    }

    float t = step_;
    pos_x = start_x_ + (end_x_ - start_x_) * t;
    pos_y = start_y_ + (end_y_ - start_y_) * t;
    float angle = start_angle_ + (end_angle_ - start_angle_) * t;
    dir_x = std::cos(angle);
    dir_y = std::sin(angle);
}
