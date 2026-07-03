#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Texture {
public:
    Texture() = default;

    void create(int w, int h);
    void set_pixel(int x, int y, uint32_t c);

    bool load_png(const std::string& path);

    int width()  const { return w_; }
    int height() const { return h_; }

    uint32_t pixel(int x, int y) const;
    uint32_t* data() { return pixels_.data(); }
    const uint32_t* data() const { return pixels_.data(); }

private:
    int w_ = 0, h_ = 0;
    std::vector<uint32_t> pixels_;
};
