#pragma once

#include <functional>
#include <vector>

class Scheduler {
public:
    using ID = size_t;

    ID after(float delay, std::function<void()> callback);
    ID every(float interval, std::function<void()> callback);
    bool cancel(ID id);
    void clear();
    void update(float dt);
    bool empty() const;

private:
    struct Event {
        float remaining;
        float interval;
        std::function<void()> callback;
        ID id = 0;
        bool active = true;
    };
    std::vector<Event> events_;
    ID next_id_ = 0;
};
