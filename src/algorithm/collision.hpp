#pragma once

class MazeGenerator;

namespace collision {
    bool can_move(const MazeGenerator& maze, int x, int y, int dir);
}
