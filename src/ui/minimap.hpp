#pragma once

#include <cstdint>
#include <utility>
#include <vector>

class Entity;
class Framebuffer;
class MazeGenerator;

static constexpr int MINIMAP_SCALE = 160;

void draw_minimap(Framebuffer& fb, const MazeGenerator& maze,
                  const std::vector<Entity*>& entities,
                  float pos_x, float pos_y, float dir_x, float dir_y,
                  const std::vector<std::pair<int,int>>* path = nullptr);
