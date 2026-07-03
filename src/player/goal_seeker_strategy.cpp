#include "goal_seeker_strategy.hpp"
#include <cstdlib>
#include "algorithm/maze.hpp"

#include <cstdio>
#include "config.hpp"

static const char* dir_name(int d) {
    static const char* names[] = {"N", "E", "S", "W"};
    return names[d];
}

void GoalSeekerStrategy::set_path(int from_x, int from_y,
                                   std::vector<std::pair<int,int>> path) {
    path_ = std::move(path);
    path_idx_ = 1;
    path_done_ = false;
    if (path_idx_ < static_cast<int>(path_.size())) {
        int tx = path_[path_idx_].first;
        int ty = path_[path_idx_].second;
        next_dir_ = (tx > from_x) ? 1 : (tx < from_x) ? 3 : (ty < from_y) ? 0 : 2;
    }
    walker_.teleport(from_x, from_y, next_dir_);
}

void GoalSeekerStrategy::plan_next(const MazeGenerator& maze) {
    (void)maze;
    if (path_idx_ < 0 || path_idx_ >= static_cast<int>(path_.size())) {
        path_idx_ = -1;
        path_done_ = true;
        return;
    }

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
    } else {
        // Path invalid or finished
        path_idx_ = -1;
    }
}

void GoalSeekerStrategy::update(float dt, float& pos_x, float& pos_y,
                                 float& dir_x, float& dir_y, MazeGenerator& maze,
                                 float speed) {
    if (walker_.finished()) return;

    if (path_idx_ < 0) {
        walker_.hold_position();
        walker_.update(pos_x, pos_y, dir_x, dir_y, speed * dt, nullptr);
        return;
    }

    // Kickstart if idle
    if (!walker_.advancing())
        plan_next(maze);

    walker_.update(pos_x, pos_y, dir_x, dir_y, speed * dt,
                   [this, &maze]() -> bool {
                       if (maze.is_finish(walker_.cell_x(), walker_.cell_y())) {
                           walker_.set_finished(true);
                           if (cfg.debug_mode())
                               printf("[SEEKER] SOLVED in %d steps!\n", walker_.steps());
                           return true;
                       }
                       plan_next(maze);
                       return false;
                   });
}

const std::vector<std::pair<int,int>>* GoalSeekerStrategy::path() const {
    if (path_idx_ >= 0 && !path_.empty())
        return &path_;
    return nullptr;
}

void GoalSeekerStrategy::reset(float pos_x, float pos_y, float dir_x, float dir_y) {
    walker_.reset();
    path_.clear();
    path_idx_ = -1;
    path_done_ = false;
    int cx = static_cast<int>(pos_x);
    int cy = static_cast<int>(pos_y);
    int ndir = 1;
    if (dir_x > 0.5f)      ndir = 1;
    else if (dir_x < -0.5f) ndir = 3;
    else if (dir_y < -0.5f) ndir = 0;
    else if (dir_y > 0.5f)  ndir = 2;
    walker_.teleport(cx, cy, ndir);
}
