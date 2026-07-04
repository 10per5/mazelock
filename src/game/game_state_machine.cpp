#include "game_state_machine.hpp"
#include "cfg/config.hpp"
#include "game/singleton.hpp"
#include "player/player.hpp"
#include "algorithm/scheduler.hpp"

#include <algorithm>

bool GameStateMachine::consume_regenerate_flag() {
    bool r = regenerate_flag_;
    regenerate_flag_ = false;
    return r;
}

void GameStateMachine::lock() {
    if (state_ == State::RUNNING)
        state_ = State::LOCKED;
}

void GameStateMachine::unlock() {
    if (state_ == State::LOCKED)
        state_ = State::RUNNING;
}

void GameStateMachine::update(float dt, Player& player, Scheduler& scheduler) {
    switch (state_) {
    case State::STARTING: {
        float grow = cfg.wall_grow_time();
        wall_height_ = std::min(1.0f, game_time_ / grow);
        game_time_ += dt;
        if (wall_height_ >= 1.0f) {
            state_ = State::RUNNING;
            game_time_ = 0.0f;
        }
        break;
    }

    case State::RUNNING:
    case State::LOCKED:
        player.update(dt);
        scheduler.update(dt);
        if (state_ == State::RUNNING && player.finished()) {
            state_ = State::FINISHED;
            finished_display_ = 0.0f;
            g_logger->debug("*** Maze solved! ***");
        }
        break;

    case State::FINISHED: {
        float grow = cfg.wall_grow_time();
        finished_display_ += dt;
        wall_height_ = std::max(0.0f, 1.0f - finished_display_ / grow);
        if (finished_display_ >= cfg.restart_delay()) {
            state_ = State::STARTING;
            game_time_ = 0.0f;
            wall_height_ = 0.0f;
            regenerate_flag_ = true;
        }
        break;
    }
    }
}

void GameStateMachine::reset() {
    state_ = State::STARTING;
    game_time_ = 0.0f;
    wall_height_ = 0.0f;
    finished_display_ = 0.0f;
}
