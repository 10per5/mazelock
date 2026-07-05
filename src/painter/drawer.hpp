#pragma once

#include <cstdint>
#include <vector>

class Framebuffer;
class Raycaster;

class Drawer {
    std::vector<uint32_t> row_buf_;
    std::vector<int> x_lut_;
public:
    void frame(Framebuffer& fb, Raycaster& raycaster,
               float wall_height,
               uint32_t overlay_color = 0, float overlay_alpha = 0.0f);
};
