#include "player.hpp"
#include "auto_walk_strategy.hpp"
#include "manual_walk_strategy.hpp"
#include "goal_seeker_strategy.hpp"
#include "pathfind.hpp"
#include "config.hpp"
#include "algorithm/maze.hpp"
#include "entity/entity_thread.hpp"

Player::Player()
    : autowalk_(std::make_unique<AutoWalkStrategy>())
    , manual_(std::make_unique<ManualWalkStrategy>())
    , seeker_(std::make_unique<GoalSeekerStrategy>())
    , current_(autowalk_.get()) {}

Player::~Player() = default;

void Player::init(MazeGenerator& maze, EntityThread& entities) {
    maze_ = &maze;
    entities_ = &entities;
    ai_speed_ = cfg.ai_speed();
    player_speed_ = 3.0f;

    auto consume_check = [this](int x, int y) -> bool {
        return entities_->consume_animal_at(x, y, *maze_, pos_x_, pos_y_);
    };
    autowalk_->set_consume_check(consume_check);
    manual_->set_consume_check(consume_check);
}

void Player::switch_to(WalkStrategy* s) {
    if (current_ == s) return;
    s->reset(pos_x_, pos_y_, dir_x_, dir_y_);
    current_ = s;
}

void Player::update(float dt) {
    if (current_->finished()) return;

    float speed = (current_ == manual_.get()) ? player_speed_ : ai_speed_;
    speed *= speed_mult_;

    current_->update(dt, pos_x_, pos_y_, dir_x_, dir_y_, *maze_, speed);

    // Cell tracking
    int cx = current_->cell_x();
    int cy = current_->cell_y();

    entities_->consume_coin_at(cx, cy, *maze_);

    if (cx != prev_cx_ || cy != prev_cy_) {
        auto& c = maze_->cell(cx, cy);
        if (c.visited < 255) ++c.visited;
        prev_cx_ = cx;
        prev_cy_ = cy;
    }

    entities_->update(dt, *maze_, cx, cy, pos_x_, pos_y_, dir_x_, dir_y_);

    // When GoalSeeker path is exhausted, fall back to autowalk
    if (current_ == seeker_.get() && seeker_->path_completed()) {
        switch_to(autowalk_.get());
        if (cfg.debug_mode())
            printf("[PATH] completed, fallback to autowalk\n");
    }

    // Auto-repeat in manual mode
    if (current_ == manual_.get() && held_dir_ != 0 && !current_->advancing()) {
        switch (held_dir_) {
        case 1: manual_->move_forward(*maze_);  break;
        case 2: manual_->move_back(*maze_);     break;
        case 3: manual_->turn_left();           break;
        case 4: manual_->turn_right();          break;
        }
    }
}

bool Player::move_forward() {
    switch_to(manual_.get());
    return manual_->move_forward(*maze_);
}

bool Player::move_back() {
    switch_to(manual_.get());
    return manual_->move_back(*maze_);
}

void Player::turn_left() {
    switch_to(manual_.get());
    manual_->turn_left();
}

void Player::turn_right() {
    switch_to(manual_.get());
    manual_->turn_right();
}

void Player::enable_autowalk() {
    switch_to(autowalk_.get());
    if (cfg.debug_mode())
        printf("[AUTOWALK] enabled\n");
}

void Player::pathfind_to_finish() {
    int cx = static_cast<int>(pos_x_);
    int cy = static_cast<int>(pos_y_);
    auto path = pathfind(*maze_, cx, cy, maze_->width() - 1, maze_->height() - 1);
    if (!path.empty()) {
        switch_to(seeker_.get());
        seeker_->set_path(cx, cy, std::move(path));
        if (cfg.debug_mode())
            printf("[PATH] found path of %zu cells\n", seeker_->path()->size());
    }
}

void Player::reset() {
    pos_x_ = 0.5f; pos_y_ = 0.5f;
    dir_x_ = 1.0f; dir_y_ = 0.0f;
    prev_cx_ = -1;
    prev_cy_ = -1;
    held_dir_ = 0;
    speed_mult_ = 1.0f;

    autowalk_->reset(pos_x_, pos_y_, dir_x_, dir_y_);
    manual_->reset(pos_x_, pos_y_, dir_x_, dir_y_);
    seeker_->reset(pos_x_, pos_y_, dir_x_, dir_y_);
    current_ = autowalk_.get();
}
