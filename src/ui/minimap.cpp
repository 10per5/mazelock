#include "minimap.hpp"
#include "config.hpp"
#include "entity/entity.hpp"
#include "graphics/framebuffer.hpp"
#include "algorithm/maze.hpp"

#include <algorithm>
#include <unordered_set>

struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const {
        return p.first ^ (p.second << 11);
    }
};

void draw_minimap(Framebuffer& fb, const MazeGenerator& maze,
                  const std::vector<Entity*>& entities,
                  float pos_x, float pos_y, float dir_x, float dir_y,
                  const std::vector<std::pair<int,int>>* path) {
    if (!cfg.minimap()) return;

    const int MARGIN = 8;
    int ox = fb.xres() - MINIMAP_SCALE - MARGIN;
    int oy = MARGIN;

    int mw = maze.width();
    int mh = maze.height();
    int cs = std::max(1, std::min(MINIMAP_SCALE / mw, MINIMAP_SCALE / mh));
    int pw = mw * cs;
    int ph = mh * cs;
    int x_off = ox + (MINIMAP_SCALE - pw) / 2;
    int y_off = oy + (MINIMAP_SCALE - ph) / 2;

    for (int py = 0; py < MINIMAP_SCALE; ++py)
        for (int px = 0; px < MINIMAP_SCALE; ++px)
            fb.put_pixel(ox + px, oy + py, 0x88000000);

    // Build path set before drawing so both layers can reference it
    std::unordered_set<std::pair<int,int>, PairHash> path_set;
    if (path && !path->empty())
        path_set = std::unordered_set<std::pair<int,int>, PairHash>(path->begin(), path->end());

    // Draw path fill (magenta) — smaller centred square so it doesn't cover the full block
    int inner = std::max(1, cs - 2);
    int off = (cs - inner) / 2;
    for (int cy = 0; cy < mh; ++cy)
        for (int cx = 0; cx < mw; ++cx)
            if (path_set.count({cx, cy}))
                for (int ly = 0; ly < inner; ++ly)
                    for (int lx = 0; lx < inner; ++lx)
                        fb.put_pixel(x_off + cx * cs + off + lx,
                                     y_off + cy * cs + off + ly, 0xFFFF00FF);

    for (int cy = 0; cy < mh; ++cy) {
        for (int cx = 0; cx < mw; ++cx) {
            bool on_path = path_set.count({cx, cy});
            int v = maze.cell(cx, cy).visited;
            int tint = std::min(v * 36, 0x99);
            uint32_t cell_color = 0xFF000000 | ((0x22 + tint) << 16) | ((0x22 + tint) << 8) | (0x22 + tint);

            uint32_t entity_color = 0;
            for (const auto& e : entities) {
                if (e->occupies(cx, cy)) {
                    entity_color = e->minimap_color();
                    break;
                }
            }

            for (int ly = 0; ly < cs; ++ly) {
                for (int lx = 0; lx < cs; ++lx) {
                    bool wall = false;
                    if (lx == 0     && maze.is_wall(cx, cy, 3)) wall = true;
                    if (ly == 0     && maze.is_wall(cx, cy, 0)) wall = true;
                    if (lx == cs - 1 && maze.is_wall(cx, cy, 1)) wall = true;
                    if (ly == cs - 1 && maze.is_wall(cx, cy, 2)) wall = true;

                    // Entity marker — draw a smaller centred square
                    bool in_entity = entity_color && lx >= off && lx < off + inner
                                                  && ly >= off && ly < off + inner;

                    uint32_t color;
                    if (wall) {
                        color = 0xFFFFFFFF;
                    } else if (in_entity) {
                        color = entity_color;
                    } else if (on_path) {
                        continue; // leave pink from path fill
                    } else {
                        color = cell_color;
                    }
                    fb.put_pixel(x_off + cx * cs + lx, y_off + cy * cs + ly, color);
                }
            }
        }
    }

    int px = x_off + static_cast<int>(pos_x * cs);
    int py = y_off + static_cast<int>(pos_y * cs);

    for (int i = 1; i <= cs; ++i) {
        int dx = px + static_cast<int>(dir_x * i);
        int dy = py + static_cast<int>(dir_y * i);
        if (dx >= x_off && dx < x_off + pw && dy >= y_off && dy < y_off + ph)
            fb.put_pixel(dx, dy, 0xFF00FF00);
    }

    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            fb.put_pixel(px + dx, py + dy, 0xFF00FF00);
}
