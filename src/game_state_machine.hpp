#pragma once

class Player;
class Scheduler;

class GameStateMachine {
public:
    enum class State {
        STARTING,
        RUNNING,
        FINISHED,
        LOCKED,
    };

    State state() const { return state_; }
    float wall_height() const { return wall_height_; }
    bool consume_regenerate_flag();

    void lock();
    void unlock();
    void update(float dt, Player& player, Scheduler& scheduler);
    void reset();

private:
    State state_ = State::STARTING;
    float wall_height_ = 0.0f;
    float game_time_ = 0.0f;
    float finished_display_ = 0.0f;
    bool regenerate_flag_ = false;
};
