#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "../walker.hpp"

class MazeGenerator;

class WalkStrategy {
public:
    virtual ~WalkStrategy() = default;

    virtual void update(float dt, float& pos_x, float& pos_y,
                        float& dir_x, float& dir_y, MazeGenerator& maze,
                        float speed) = 0;

    Walker& walker() { return walker_; }
    const Walker& walker() const { return walker_; }
    bool finished() const { return walker_.finished(); }
    int cell_x() const { return walker_.cell_x(); }
    int cell_y() const { return walker_.cell_y(); }
    int steps() const { return walker_.steps(); }
    bool advancing() const { return walker_.advancing(); }

    virtual const std::vector<std::pair<int,int>>* path() const { return nullptr; }

    virtual void reset(float pos_x, float pos_y, float dir_x, float dir_y) = 0;
    void set_god_mode(bool g) { walker_.set_god_mode(g); }
    void set_maze(MazeGenerator* m) { walker_.set_maze(m); }

protected:
    Walker walker_;
};
