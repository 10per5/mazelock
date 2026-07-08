#include "raycaster.hpp"
#include "cfg/config.hpp"
#include "texture.hpp"
#include "texture_manager.hpp"
#include "algorithm/maze.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

Raycaster::Raycaster() : color_buffer_(RENDER_W * RENDER_H), depth_buffer_(RENDER_W) {}

void Raycaster::set_textures(const TextureManager& tm) {
    wall_tex_    = &tm.wall();
    floor_tex_   = &tm.floor();
    ceiling_tex_ = &tm.ceiling();
}

void Raycaster::cast(const MazeGenerator& maze, float pos_x, float pos_y, float dir_x, float dir_y,
                     float wall_height) {
    std::fill(color_buffer_.begin(), color_buffer_.end(), 0xFF000000);

    const int maze_w = maze.width();
    const int maze_h = maze.height();
    int textured = cfg.textured_floor();

    float plane_x = -dir_y * FOV;
    float plane_y =  dir_x * FOV;
    float pos_z = RENDER_H / 2.0f;

    int wt_w = wall_tex_    ? wall_tex_->width()    : 0;
    int wt_h = wall_tex_    ? wall_tex_->height()   : 0;
    int fl_w = floor_tex_   ? floor_tex_->width()   : 0;
    int fl_h = floor_tex_   ? floor_tex_->height()  : 0;
    int cl_w = ceiling_tex_ ? ceiling_tex_->width() : 0;
    int cl_h = ceiling_tex_ ? ceiling_tex_->height(): 0;

    float floor_scale = cfg.floor_tile_scale();
    float ceil_scale  = cfg.ceiling_tile_scale();

    for (int x = 0; x < RENDER_W; ++x) {
        float camera_x = 2.0f * x / static_cast<float>(RENDER_W) - 1.0f;
        float ray_dir_x = dir_x + plane_x * camera_x;
        float ray_dir_y = dir_y + plane_y * camera_x;

        int map_x = static_cast<int>(pos_x);
        int map_y = static_cast<int>(pos_y);

        float delta_dist_x = (ray_dir_x == 0.0f) ? 1e30f : std::abs(1.0f / ray_dir_x);
        float delta_dist_y = (ray_dir_y == 0.0f) ? 1e30f : std::abs(1.0f / ray_dir_y);

        int step_x, step_y;
        float side_dist_x, side_dist_y;

        if (ray_dir_x < 0) {
            step_x = -1;
            side_dist_x = (pos_x - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0f - pos_x) * delta_dist_x;
        }

        if (ray_dir_y < 0) {
            step_y = -1;
            side_dist_y = (pos_y - map_y) * delta_dist_y;
        } else {
            step_y = 1;
            side_dist_y = (map_y + 1.0f - pos_y) * delta_dist_y;
        }

        int hit = 0;
        int side = 0;

        while (hit == 0) {
            if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }
            if (map_x < 0 || map_x >= maze_w || map_y < 0 || map_y >= maze_h) {
                hit = 1;
                depth_buffer_[x] = 1e30f;
                continue;
            }
            int wall_dir;
            if (side == 0) {
                wall_dir = (step_x > 0) ? 3 : 1;
            } else {
                wall_dir = (step_y > 0) ? 0 : 2;
            }
            if (maze.cell(map_x, map_y).walls[wall_dir]) {
                hit = 1;
            }
        }

        float perp_dist;
        if (side == 0) {
            perp_dist = (map_x - pos_x + (1 - step_x) / 2.0f) / ray_dir_x;
        } else {
            perp_dist = (map_y - pos_y + (1 - step_y) / 2.0f) / ray_dir_y;
        }
        depth_buffer_[x] = perp_dist;

        int line_height = static_cast<int>(RENDER_H / perp_dist * wall_height);
        int draw_start = -line_height / 2 + RENDER_H / 2;
        if (draw_start < 0) draw_start = 0;
        int draw_end = line_height / 2 + RENDER_H / 2;
        if (draw_end >= RENDER_H) draw_end = RENDER_H - 1;

        float wall_x;
        if (side == 0) {
            wall_x = pos_y + perp_dist * ray_dir_y;
        } else {
            wall_x = pos_x + perp_dist * ray_dir_x;
        }
        wall_x -= static_cast<int>(wall_x);

        int wall_tx = (wt_w > 0) ? static_cast<int>(wall_x * wt_w) % wt_w : 0;
        float wall_y_step = (draw_end > draw_start) ? 1.0f / (draw_end - draw_start + 1) : 1.0f;

        for (int y = 0; y < RENDER_H; ++y) {
            if (y < draw_start) {
                if (textured && cl_w > 0 && cl_h > 0) {
                    float p = pos_z - y;
                    if (p > 0.0f) {
                        float row_dist = pos_z / p;
                        float wx = pos_x + row_dist * ray_dir_x;
                        float wy = pos_y + row_dist * ray_dir_y;
                        int tx = static_cast<int>(wx * ceil_scale) % cl_w;
                        int ty = static_cast<int>(wy * ceil_scale) % cl_h;
                        if (tx < 0) tx += cl_w;
                        if (ty < 0) ty += cl_h;
                        color_buffer_[y * RENDER_W + x] = ceiling_tex_->pixel(tx, ty);
                    } else {
                        color_buffer_[y * RENDER_W + x] = 0xFF666666;
                    }
                } else {
                    color_buffer_[y * RENDER_W + x] = 0xFF666666;
                }
            } else if (y <= draw_end) {
                if (wt_w > 0 && wt_h > 0) {
                    float wall_y = (y - draw_start) * wall_y_step;
                    int ty = static_cast<int>(wall_y * wt_h) % wt_h;
                    uint32_t c = wall_tex_->pixel(wall_tx, ty);
                    if (side == 1) {
                        int r = ((c >> 16) & 0xFF) * 3 / 4;
                        int g = ((c >>  8) & 0xFF) * 3 / 4;
                        int b = (c        & 0xFF) * 3 / 4;
                        c = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                    color_buffer_[y * RENDER_W + x] = c;
                } else {
                    uint32_t c = (side == 0) ? 0xFFAAAAAA : 0xFFCCCCCC;
                    int check_tx = static_cast<int>(wall_x * 8);
                    float wall_y = (y - draw_start) * wall_y_step;
                    int ty = static_cast<int>(wall_y * 8);
                    if ((check_tx + ty) & 1) {
                        int r = ((c >> 16) & 0xFF) * 3 / 4;
                        int g = ((c >>  8) & 0xFF) * 3 / 4;
                        int b = (c        & 0xFF) * 3 / 4;
                        c = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                    color_buffer_[y * RENDER_W + x] = c;
                }
            } else {
                if (textured && fl_w > 0 && fl_h > 0) {
                    float p = y - pos_z;
                    if (p > 0.0f) {
                        float row_dist = pos_z / p;
                        float wx = pos_x + row_dist * ray_dir_x;
                        float wy = pos_y + row_dist * ray_dir_y;
                        int tx = static_cast<int>(wx * floor_scale) % fl_w;
                        int ty = static_cast<int>(wy * floor_scale) % fl_h;
                        if (tx < 0) tx += fl_w;
                        if (ty < 0) ty += fl_h;
                        uint32_t fc = floor_tex_->pixel(tx, ty);
                        int cx = static_cast<int>(std::floor(wx));
                        int cy = static_cast<int>(std::floor(wy));
                        if (ty % 8 == 0 && cx >= 0 && cx < maze_w && cy >= 0 && cy < maze_h) {
                            if (maze.cell(cx, cy).walls[0]) {
                                int r = ((fc >> 16) & 0xFF) * 2 / 3;
                                int g = ((fc >>  8) & 0xFF) * 2 / 3;
                                int b = (fc        & 0xFF) * 2 / 3;
                                fc = 0xFF000000 | (r << 16) | (g << 8) | b;
                            }
                        }
                        color_buffer_[y * RENDER_W + x] = fc;
                    } else {
                        color_buffer_[y * RENDER_W + x] = 0xFF333333;
                    }
                } else {
                    color_buffer_[y * RENDER_W + x] = 0xFF333333;
                }
            }
        }
    }
}
