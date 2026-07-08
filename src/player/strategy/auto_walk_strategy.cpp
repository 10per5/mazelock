#include "auto_walk_strategy.hpp"
#include "game/singleton.hpp"
#include "algorithm/maze.hpp"

#include <algorithm>
#include <cstdlib>

static const char* dir_name(int d) {
    static const char* names[] = {"N", "E", "S", "W"};
    return names[d];
}

// -----------------------------------------------------------------------
// Decision planning (AI layer)
// -----------------------------------------------------------------------

void AutoWalkStrategy::plan_next_step(const MazeGenerator& maze) {
    // In manual mode — freeze walker so it doesn't drift between inputs.
    if (walker_.freeze_when_idle()) {
        if (!walker_.advancing())
            walker_.hold_position();
        return;
    }

    // Reverse sequence (from animal hit) — choose the shortest viable
    // turn that leaves the walker in an open direction, preferring
    // 90° > 180° > 270° > 360° (full circle back to original).
    if (reversing_) {
        if (reverse_phase_ == 0) {
            // Peek ahead from current cell to decide target
            int cur = walker_.direction();
            bool w90  = maze.is_wall(walker_.cell_x(), walker_.cell_y(), (cur + 3) % 4);
            bool w180 = maze.is_wall(walker_.cell_x(), walker_.cell_y(), (cur + 2) % 4);
            bool w270 = maze.is_wall(walker_.cell_x(), walker_.cell_y(), (cur + 1) % 4);

            if (!w180) {
                reverse_target_ = 2;    // 180° is open — normal reverse
            } else if (!w90) {
                reverse_target_ = 1;    // 180° blocked — try 90° instead
            } else if (!w270) {
                reverse_target_ = 3;    // 90° & 180° blocked — try 270°
            } else {
                reverse_target_ = 4;    // full circle back to original
            }
            g_logger->debug("[AI] reverse: target=%d (cur=%s walls 90:%d 180:%d 270:%d)",
                       reverse_target_, dir_name(cur), w90, w180, w270);
        }

        // Wait for any in-progress step to finish.
        // (turning_ stays true after a turn completes for the "force forward
        //  after turn" feature — we must not wait for it.)
        if (walker_.advancing())
            return;

        // All required turns done — resume normal navigation
        if (reverse_phase_ >= reverse_target_) {
            g_logger->debug("[AI] reverse: done (%s)",
                       dir_name(walker_.direction()));
            reversing_ = false;
            reverse_phase_ = 0;
            reverse_target_ = 2;
            return;
        }

        // One more 90° left turn
        ++reverse_phase_;
        walker_.plan_turn((walker_.direction() + 3) % 4);
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

    // After the walk-away phase (reverse_steps_ counted down by
    // on_step_complete), turn back toward navigable direction via
    // phased 90° left turns.  Placed before the "turning → move"
    // check so turnaround turns are not preempted by a forward move.
    if (reverse_steps_ == 0) {
        if (!walker_.turnaround_active()) {
            walker_.start_turnaround(maze);
        }
        if (walker_.advancing())
            return;
        if (walker_.advance_turnaround()) {
            reverse_steps_ = -1;  // prevent re-entry
            return;               // fall through to normal navigation
        }
        return;
    }

    // After completing a turn, always move forward (don't re-evaluate)
    if (walker_.turning() && !reversing_) {
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

    if (random_explore_dist_(rng_) == 0) {
        if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), left_dir)) {
            next_dir = left_dir;
        } else if (!maze.is_wall(walker_.cell_x(), walker_.cell_y(), right_dir)) {
            next_dir = right_dir;
        }
    }

    g_logger->debug("[AI] (%d,%d) %s->%s  R:%s F:%s L:%s%s",
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

// -----------------------------------------------------------------------
// Step-completion callback — called by the walker every time a cell is entered
// Returns true to request an early bail (pause or finish)
// -----------------------------------------------------------------------

bool AutoWalkStrategy::on_step_complete(const MazeGenerator& maze) {
    // Don't plan new moves during the post-animal pause
    if (reverse_pause_ > 0)
        return false;

    // Count forward steps for the walk-away phase (skip reverse turn steps)
    if (!reversing_ && reverse_steps_ > 0)
        --reverse_steps_;

    plan_next_step(maze);
    return pause_ > 0;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void AutoWalkStrategy::set_path(int from_x, int from_y, std::vector<std::pair<int,int>> path) {
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
}

void AutoWalkStrategy::clear_path() {
    path_.clear();
    path_idx_ = -1;
}

void AutoWalkStrategy::reset(float pos_x, float pos_y, float dir_x, float dir_y) {
    rng_.seed(std::random_device{}());
    pause_ = 0;
    reverse_pause_ = 0;
    reversing_ = false;
    reverse_phase_ = 0;
    reverse_target_ = 2;
    reverse_steps_ = -1;
    path_.clear();
    path_idx_ = -1;
    walker_.reset();
    int cx = static_cast<int>(pos_x);
    int cy = static_cast<int>(pos_y);
    int ndir = 1;
    if (dir_x > 0.5f)      ndir = 1;
    else if (dir_x < -0.5f) ndir = 3;
    else if (dir_y < -0.5f) ndir = 0;
    else if (dir_y > 0.5f)  ndir = 2;
    walker_.teleport(cx, cy, ndir);
}

// Main update

void AutoWalkStrategy::update(float dt, float& pos_x, float& pos_y,
                               float& dir_x, float& dir_y, MazeGenerator& maze,
                               float speed) {
    if (walker_.finished()) return;
    if (pause_ > 0) { --pause_; return; }

    // Set up animal-hit callback once
    if (!on_animal_setup_) {
        walker_.set_on_animal([this]() {
            if (reversing_) return;     // already reversing — ignore repeat hits
            reverse_pause_ = 45;
            reversing_ = true;
            reverse_phase_ = 0;
            reverse_steps_ = 5 + std::uniform_int_distribution<int>(0, 1)(rng_);
        });
        on_animal_setup_ = true;
    }

    // Hold position so the flee animation is visible before turning.
    // Counts AFTER consume_pause_ expires (sequential, not overlapping).
    if (reverse_pause_ > 0) {
        if (!walker_.consuming()) {
            --reverse_pause_;
            if (!walker_.advancing())
                walker_.hold_position();
        }
    } else if (!walker_.advancing() && !walker_.consuming()) {
        plan_next_step(maze);
    }

    walker_.update(pos_x, pos_y, dir_x, dir_y, speed * dt,
                   [this, &maze]() { return on_step_complete(maze); });
}
