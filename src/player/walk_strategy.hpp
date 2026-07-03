#pragma once

#include <functional>
#include <utility>
#include <vector>

class MazeGenerator;

class WalkStrategy {
public:
    virtual ~WalkStrategy() = default;

    virtual void update(float dt, float& pos_x, float& pos_y,
                        float& dir_x, float& dir_y, MazeGenerator& maze,
                        float speed) = 0;

    virtual bool finished() const = 0;
    virtual int cell_x() const = 0;
    virtual int cell_y() const = 0;
    virtual int steps() const = 0;
    virtual bool advancing() const = 0;

    virtual const std::vector<std::pair<int,int>>* path() const { return nullptr; }

    virtual void reset(float pos_x, float pos_y, float dir_x, float dir_y) = 0;
    virtual void set_consume_check(std::function<bool(int,int)>) {}
};
