#include "maze.hpp"

#include <array>

#include "game/singleton.hpp"

#include <random>
#include <string>

MazeGenerator::MazeGenerator(int w, int h) : width_(w), height_(h), grid_(w * h) {
    generate();
}

void MazeGenerator::generate() {
    for (auto& c : grid_) {
        c.walls = {true, true, true, true};
        c.object = OBJECT_NONE;
        c.visited = 0;
    }

    std::mt19937 rng(std::random_device{}());
    std::vector<bool> visited(width_ * height_, false);
    std::vector<int> stack;

    int cx = 0, cy = 0;
    visited[cy * width_ + cx] = true;
    stack.push_back(cy * width_ + cx);

    static constexpr int dx[4] = { 0, 1, 0, -1 };
    static constexpr int dy[4] = {-1, 0, 1,  0 };
    static constexpr int opp[4] = {2, 3, 0, 1};

    while (!stack.empty()) {
        int idx = stack.back();
        int x = idx % width_;
        int y = idx / width_;

        std::array<int, 4> nb;
        int ncount = 0;
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx >= 0 && nx < width_ && ny >= 0 && ny < height_ && !visited[ny * width_ + nx]) {
                nb[ncount++] = d;
            }
        }

        if (ncount > 0) {
            int d = nb[rng() % ncount];
            int nx = x + dx[d], ny = y + dy[d];
            grid_[y * width_ + x].walls[d] = false;
            grid_[ny * width_ + nx].walls[opp[d]] = false;
            visited[ny * width_ + nx] = true;
            stack.push_back(ny * width_ + nx);
        } else {
            stack.pop_back();
        }
    }

    grid_[0 * width_ + 0].object = OBJECT_START;
    grid_[(height_ - 1) * width_ + (width_ - 1)].object = OBJECT_FINISH;

    std::uniform_int_distribution<int> xd(0, width_ - 1);
    std::uniform_int_distribution<int> yd(0, height_ - 1);
    int placed = 0;
    while (placed < 10) {
        int tx = xd(rng), ty = yd(rng);
        if (grid_[ty * width_ + tx].object != OBJECT_NONE) continue;
        grid_[ty * width_ + tx].object = OBJECT_COIN;
        ++placed;
    }

    placed = 0;
    int animal_count = 5;
    while (placed < animal_count) {
        int tx = xd(rng), ty = yd(rng);
        if (grid_[ty * width_ + tx].object != OBJECT_NONE) continue;
        grid_[ty * width_ + tx].object = OBJECT_ANIMAL;
        ++placed;
    }
}

void MazeGenerator::print() const {
    std::string buf;
    // top border
    buf += '+';
    for (int x = 0; x < width_; ++x) buf += "---+";
    g_logger->log("%s", buf.c_str());

    for (int y = 0; y < height_; ++y) {
        buf.clear();
        buf += '|';
        for (int x = 0; x < width_; ++x) {
            const auto& c = grid_[y * width_ + x];
            char ch = ' ';
            if (c.object == OBJECT_START)   ch = 'S';
            else if (c.object == OBJECT_FINISH)  ch = 'F';
            else if (c.object == OBJECT_COIN) ch = 'C';
            else if (c.object == OBJECT_ANIMAL)  ch = 'A';
            buf += ' ';
            buf += ch;
            buf += ' ';
            buf += c.walls[1] ? '|' : ' ';
        }
        g_logger->log("%s", buf.c_str());

        buf.clear();
        buf += '+';
        for (int x = 0; x < width_; ++x) {
            const auto& c = grid_[y * width_ + x];
            buf += c.walls[2] ? "---" : "   ";
            buf += '+';
        }
        g_logger->log("%s", buf.c_str());
    }
}
