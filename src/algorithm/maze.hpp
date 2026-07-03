#pragma once

#include <array>
#include <vector>

enum CellObject {
    OBJECT_NONE,
    OBJECT_START,
    OBJECT_FINISH,
    OBJECT_COIN,
    OBJECT_ANIMAL
};

struct Cell {
    std::array<bool, 4> walls{true, true, true, true};
    CellObject object = OBJECT_NONE;
    int visited = 0;
};

class MazeGenerator {
    int width_, height_;
    std::vector<Cell> grid_;

    void generate();

public:
    MazeGenerator(int w, int h);

    void regenerate() { generate(); }

    int width()  const { return width_; }
    int height() const { return height_; }

    const Cell& cell(int x, int y) const {
        return grid_[y * width_ + x];
    }
    Cell& cell(int x, int y) {
        return grid_[y * width_ + x];
    }

    void clear_object(int x, int y) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_)
            grid_[y * width_ + x].object = OBJECT_NONE;
    }

    bool is_wall(int x, int y, int dir) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return true;
        return grid_[y * width_ + x].walls[dir];
    }

    bool is_finish(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
        return grid_[y * width_ + x].object == OBJECT_FINISH;
    }

    CellObject get_object(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return OBJECT_NONE;
        return grid_[y * width_ + x].object;
    }

    void print() const;
};
