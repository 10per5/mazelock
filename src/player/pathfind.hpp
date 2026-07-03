#pragma once

#include <utility>
#include <vector>

class MazeGenerator;

std::vector<std::pair<int,int>> pathfind(
    const MazeGenerator& maze, int sx, int sy, int ex, int ey);
