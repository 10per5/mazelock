#include "pathfind.hpp"
#include "algorithm/maze.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

std::vector<std::pair<int,int>> pathfind(
    const MazeGenerator& maze, int sx, int sy, int ex, int ey)
{
    struct Node {
        int x, y;
        float g, f;
        int px, py; // parent
        bool closed = false;
    };

    int w = maze.width();
    int h = maze.height();

    auto idx = [w](int x, int y) { return y * w + x; };

    static constexpr int dx[4] = { 0, 1, 0, -1 };
    static constexpr int dy[4] = {-1, 0, 1,  0 };

    // Simple open list — linear scan is fine for small mazes
    std::vector<Node> nodes(w * h);
    std::vector<int> open;

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            nodes[idx(x, y)] = {x, y, std::numeric_limits<float>::max(), 0, -1, -1, false};

    auto& start = nodes[idx(sx, sy)];
    start.g = 0;
    start.f = static_cast<float>(std::abs(sx - ex) + std::abs(sy - ey));
    start.px = -1;
    start.py = -1;
    open.push_back(idx(sx, sy));

    while (!open.empty()) {
        // Find lowest f in open list
        int best_i = 0;
        for (size_t i = 1; i < open.size(); ++i)
            if (nodes[open[i]].f < nodes[open[best_i]].f)
                best_i = i;

        int cur_idx = open[best_i];
        auto& cur = nodes[cur_idx];
        open.erase(open.begin() + best_i);

        if (cur.closed) continue;
        cur.closed = true;

        if (cur.x == ex && cur.y == ey) {
            // Reconstruct path including start cell
            std::vector<std::pair<int,int>> path;
            int cx = ex, cy = ey;
            path.push_back({cx, cy});
            while (cx != sx || cy != sy) {
                auto& n = nodes[idx(cx, cy)];
                cx = n.px;
                cy = n.py;
                path.push_back({cx, cy});
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int d = 0; d < 4; ++d) {
            if (maze.is_wall(cur.x, cur.y, d)) continue;
            int nx = cur.x + dx[d];
            int ny = cur.y + dy[d];
            int nidx = idx(nx, ny);
            auto& nb = nodes[nidx];

            if (nb.closed) continue;

            float ng = cur.g + 1.0f;
            if (ng < nb.g) {
                nb.g = ng;
                nb.f = ng + static_cast<float>(std::abs(nx - ex) + std::abs(ny - ey));
                nb.px = cur.x;
                nb.py = cur.y;
                if (std::find(open.begin(), open.end(), nidx) == open.end())
                    open.push_back(nidx);
            }
        }
    }

    return {}; // No path found
}
