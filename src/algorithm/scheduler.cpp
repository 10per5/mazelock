#include "scheduler.hpp"

#include <algorithm>

Scheduler::ID Scheduler::after(float delay, std::function<void()> callback) {
    ID id = next_id_++;
    events_.push_back({delay, 0.0f, std::move(callback), id, true});
    return id;
}

Scheduler::ID Scheduler::every(float interval, std::function<void()> callback) {
    ID id = next_id_++;
    events_.push_back({interval, interval, std::move(callback), id, true});
    return id;
}

bool Scheduler::cancel(ID id) {
    for (auto& e : events_) {
        if (e.id == id && e.active) {
            e.active = false;
            return true;
        }
    }
    return false;
}

void Scheduler::clear() {
    events_.clear();
}

void Scheduler::update(float dt) {
    for (auto& e : events_) {
        if (!e.active) continue;
        e.remaining -= dt;
        if (e.remaining <= 0.0f) {
            e.callback();
            if (e.interval > 0.0f) {
                e.remaining += e.interval;
            } else {
                e.active = false;
            }
        }
    }

    events_.erase(
        std::remove_if(events_.begin(), events_.end(),
                       [](const Event& e) { return !e.active; }),
        events_.end());
}

bool Scheduler::empty() const {
    return events_.empty();
}
