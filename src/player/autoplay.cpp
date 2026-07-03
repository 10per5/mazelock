#include "autoplay.hpp"
#include "algorithm/collision.hpp"
#include "config.hpp"
#include "algorithm/maze.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

static constexpr float PI = 3.14159265358979f;

static const char* dir_name(int d) {
    static const char* names[] = {"N", "E", "S", "W"};
    return names[d];
}

// -----------------------------------------------------------------------
// Decision planning (AI layer)
// -----------------------------------------------------------------------

void AutoplayAI::plan_next_step(const MazeGenerator& maze) {
    // In manual mode — freeze walker so it doesn't drift between inputs.
    // But only when no step was just started by flush_pending (avoid overwriting targets).
    if (manual_mode_) {
        if (!walker_.advancing())
            walker_.hold_position();
        return;
    }

    // Path-following mode — advance toward next waypoint
    if (path_idx_ >= 0 && path_idx_ < static_cast<int>(path_.size())) {
        int tx = path_[path_idx_].first;
        int ty = path_[path_idx_].second;
        int dx_ = tx - walker_.cell_x(), dy_ = ty - walker_.cell_y();

        if ((std::abs(dx_) == 1 && dy_ == 0) || (dx_ == 0 && std::abs(dy_) == 1)) {
            int target_dir = (dx_ == 1) ? 1 : (dx_ == -1) ? 3 : (dy_ == -1) ? 0 : 2;

            if (target_dir == walker_.direction()) {
                ++path_idx_;
                walker_.plan_move(target_dir);
            } else {
                walker_.plan_turn(target_dir);
            }
            return;
        }
        clear_path();
    }

    // After completing a turn, always move forward (don't re-evaluate)
    if (walker_.turning()) {
        walker_.plan_move(walker_.direction());
        return;
    }

    // Right-hand rule + random exploration
    int dir = walker_.direction();
    int front_dir = dir;
    int right_dir = (dir + 1) % 4;
    int left_dir  = (dir + 3) % 4;

    int next_dir;

    if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), right_dir)) {
        next_dir = right_dir;
    } else if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), front_dir)) {
        next_dir = front_dir;
    } else if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), left_dir)) {
        next_dir = left_dir;
    } else {
        next_dir = (dir + 2) % 4;
        pause_ = PAUSE_FRAMES;
    }

    if (std::uniform_int_distribution<int>(0, 19)(rng_) == 0) {
        if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), left_dir)) {
            next_dir = left_dir;
        } else if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), right_dir)) {
            next_dir = right_dir;
        }
    }

    if (cfg.debug_mode())
        printf("[AI] (%d,%d) %s->%s  R:%s F:%s L:%s%s\n",
               walker_.cell_x(), walker_.cell_y(),
               dir_name(dir), dir_name(next_dir),
               maze.is_wall(walker_.cell_x(), walker_.cell_y(), right_dir) ? "W" : ".",
               maze.is_wall(walker_.cell_x(), walker_.cell_y(), front_dir) ? "W" : ".",
               maze.is_wall(walker_.cell_x(), walker_.cell_y(), left_dir)  ? "W" : ".",
               pause_ ? " PAUSE" : "");

    if (next_dir == dir) {
        walker_.plan_move(next_dir);
    } else {
        walker_.plan_turn(next_dir);
    }
}

int AutoplayAI::do_reverse() {
    int back_dir = (walker_.direction() + 2) % 4;
    walker_.plan_move(back_dir);
    return back_dir;
}

// -----------------------------------------------------------------------
// Step-completion callback — called by the walker every time a cell is entered
// Returns true to request an early bail (pause or finish)
// -----------------------------------------------------------------------

bool AutoplayAI::on_step_complete(const MazeGenerator& maze) {
    if (reverse_requested_) {
        reverse_requested_ = false;
        int new_dir = do_reverse();
        if (cfg.debug_mode())
            printf("[AI] REVERSE via consume at (%d,%d) now %s\n",
                   walker_.cell_x(), walker_.cell_y(), dir_name(new_dir));
        if (maze.is_finish(walker_.cell_x(), walker_.cell_y())) {
            walker_.set_finished(true);
            if (cfg.debug_mode())
                printf("[AI] SOLVED in %d steps!\n", walker_.steps());
        }
        return false;
    }

    if (!disable_animal_reverse_ && consume_check_ && consume_check_(walker_.cell_x(), walker_.cell_y())) {
        int new_dir = do_reverse();
        if (cfg.debug_mode())
            printf("[AI] REVERSE via consume at (%d,%d) now %s\n",
                   walker_.cell_x(), walker_.cell_y(), dir_name(new_dir));
        if (maze.is_finish(walker_.cell_x(), walker_.cell_y())) {
            walker_.set_finished(true);
            if (cfg.debug_mode())
                printf("[AI] SOLVED in %d steps!\n", walker_.steps());
        }
        return false;
    }

    plan_next_step(maze);

    if (maze.is_finish(walker_.cell_x(), walker_.cell_y())) {
        walker_.set_finished(true);
        if (cfg.debug_mode())
            printf("[AI] SOLVED in %d steps!\n", walker_.steps());
        return true;
    }

    return pause_ > 0;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void AutoplayAI::set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path) {
    path_ = std::move(path);
    path_idx_ = 1;
    int ndir = 1;
    if (path_idx_ < static_cast<int>(path_.size())) {
        int tx = path_[path_idx_].first;
        int ty = path_[path_idx_].second;
        ndir = (tx > from_x) ? 1 : (tx < from_x) ? 3 : (ty < from_y) ? 0 : 2;
    }
    walker_.teleport(from_x, from_y, ndir);
    pause_ = 0;
    disable_animal_reverse_ = true;
}

void AutoplayAI::clear_path() {
    path_.clear();
    path_idx_ = -1;
    disable_animal_reverse_ = false;
}

void AutoplayAI::request_reverse() {
    reverse_requested_ = true;
}

void AutoplayAI::reset() {
    rng_.seed(std::random_device{}());
    walker_.reset();
    pause_ = 0;
    reverse_requested_ = false;
    manual_mode_ = false;
    path_.clear();
    path_idx_ = -1;
    disable_animal_reverse_ = false;
}

// Manual control

bool AutoplayAI::manual_forward(const MazeGenerator& maze) {
    int dir = walker_.direction();
    if (maze.is_wall(walker_.cell_x(), walker_.cell_y(), dir)) return false;
    if (!walker_.plan_move(dir))
        walker_.restart_step();
    if (cfg.debug_mode())
        printf("[MANUAL] forward (%d,%d) %s\n",
               walker_.cell_x(), walker_.cell_y(), dir_name(dir));
    return true;
}

bool AutoplayAI::manual_back(const MazeGenerator& maze) {
    int back_dir = (walker_.direction() + 2) % 4;
    if (maze.is_wall(walker_.cell_x(), walker_.cell_y(), back_dir)) return false;
    if (!walker_.plan_move(back_dir))
        walker_.restart_step();
    if (cfg.debug_mode())
        printf("[MANUAL] back (%d,%d) now %s\n",
               walker_.cell_x(), walker_.cell_y(), dir_name(back_dir));
    return true;
}

bool AutoplayAI::manual_turn_left() {
    int new_dir = (walker_.direction() + 3) % 4;
    if (!walker_.plan_turn(new_dir))
        walker_.restart_step();
    if (cfg.debug_mode())
        printf("[MANUAL] turn left (%d,%d) now %s\n",
               walker_.cell_x(), walker_.cell_y(), dir_name(new_dir));
    return true;
}

bool AutoplayAI::manual_turn_right() {
    int new_dir = (walker_.direction() + 1) % 4;
    if (!walker_.plan_turn(new_dir))
        walker_.restart_step();
    if (cfg.debug_mode())
        printf("[MANUAL] turn right (%d,%d) now %s\n",
               walker_.cell_x(), walker_.cell_y(), dir_name(new_dir));
    return true;
}

// Main update

void AutoplayAI::update(float& pos_x, float& pos_y, float& dir_x, float& dir_y,
                        const MazeGenerator& maze, float speed) {
    if (walker_.finished()) return;
    if (pause_ > 0) { --pause_; return; }

    walker_.set_collision_check([&maze](int x, int y, int dir) {
        return collision::can_move(maze, x, y, dir);
    });

    // Kickstart: if the walker is idle (e.g. frozen by hold_position after a mode
    // switch from manual), plan the next AI move immediately so the walker doesn't
    // stall waiting for a step to complete.
    if (!walker_.advancing())
        plan_next_step(maze);

    walker_.update(pos_x, pos_y, dir_x, dir_y, speed,
                   [this, &maze]() { return on_step_complete(maze); });
}
