#pragma once

#include <cstdint>
#include <vector>

class MazeGenerator;
class Texture;
class TextureManager;

class Raycaster {
    static constexpr int RENDER_W = 320;
    static constexpr int RENDER_H = 200;
    static constexpr float FOV = 0.66f;

    std::vector<uint32_t> color_buffer_;
    std::vector<float> depth_buffer_;

public:
    Raycaster();

    int render_width()  const { return RENDER_W; }
    int render_height() const { return RENDER_H; }

    const std::vector<uint32_t>& color_buffer() const { return color_buffer_; }
    std::vector<uint32_t>& color_buffer() { return color_buffer_; }
    const std::vector<float>& depth_buffer() const { return depth_buffer_; }

    void set_textures(const TextureManager& tm);

    void cast(const MazeGenerator& maze, float pos_x, float pos_y, float dir_x, float dir_y,
              float wall_height);

private:
    const Texture* wall_tex_ = nullptr;
    const Texture* floor_tex_ = nullptr;
    const Texture* ceiling_tex_ = nullptr;
};
