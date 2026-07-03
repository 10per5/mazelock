#include "collision.hpp"
#include "algorithm/maze.hpp"

bool collision::can_move(const MazeGenerator& maze, int x, int y, int dir) {
    return !maze.is_wall(x, y, dir);
}
