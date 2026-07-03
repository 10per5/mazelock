#include "texture_manager.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>

bool TextureManager::init(const std::string& tex_dir) {
#ifdef USEPNG
    return load_from_png(tex_dir);
#else
    generate_wall();
    generate_floor();
    generate_ceiling();
    generate_coin();
    generate_rat();
    return true;
#endif
}

bool TextureManager::load_from_png(const std::string& dir) {
    bool ok = true;
    auto load = [&](Texture& t, const char* name) {
        std::string path = dir + "/" + name;
        if (!t.load_png(path)) {
            std::fprintf(stderr, "Failed to load %s\n", path.c_str());
            ok = false;
        }
    };
    load(wall_,    "wall.png");
    load(floor_,   "floor.png");
    load(ceiling_, "ceiling.png");
    load(coin_,    "coin.png");
    load(rat_,     "rat.png");
    return ok;
}

void TextureManager::generate_wall() {
    int w = 64, h = 64;
    wall_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int row = y / 16;
            int mx = (x + (row % 2 ? 8 : 0)) % 16;
            int my = y % 16;
            uint32_t c;
            if (mx == 15 || my == 15) {
                c = 0xFFC8BEB4;  // mortar
            } else {
                int v = 160 + (std::abs((x / 3) * 7 + (y / 3) * 13) % 40);
                c = 0xFF000000 | (v << 16) | ((v / 2) << 8) | (v / 3);
            }
            wall_.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_floor() {
    int w = 64, h = 64;
    floor_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int noise = std::abs((x * 7 + y * 13) * 3) % 30;
            int r = 130 + noise;
            int g = 85 + noise;
            int b = 40 + noise;
            if (y % 8 == 0) { r = 80; g = 55; b = 30; }
            floor_.set_pixel(x, y,
                0xFF000000 | (r << 16) | (g << 8) | b);
        }
    }
}

void TextureManager::generate_ceiling() {
    int w = 64, h = 64;
    ceiling_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int v = 210 + (std::abs((x / 4) * 7 + (y / 4) * 13) % 30);
            ceiling_.set_pixel(x, y,
                0xFF000000 | (v << 16) | (v << 8) | v);
        }
    }
}

void TextureManager::generate_coin() {
    int w = 32, h = 32;
    coin_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = static_cast<float>(x - 16);
            float dy = static_cast<float>(y - 16);
            float dist = std::sqrt(dx * dx + dy * dy);
            uint32_t c = 0x00000000;
            if (dist < 10.0f) {
                c = 0xFFFFC864;  // gold body
            }
            if (dist < 6.0f) {
                c = 0xFFFFDC8C;  // lighter center
            }
            if (std::abs(x - 13) < 2 && std::abs(y - 13) < 2) c = 0xFF000000;
            if (std::abs(x - 19) < 2 && std::abs(y - 13) < 2) c = 0xFF000000;
            if (std::abs(x - 16) < 3 && y == 19) c = 0xFF000000;
            coin_.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_rat() {
    int w = 32, h = 32;
    rat_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t c = 0x00000000;

            float cx = static_cast<float>(x - 16);
            float cy = static_cast<float>(y - 16);

            // Body — oval, slightly tilted
            float body_a = (cx + cy * 0.3f) / 9.0f;
            float body_b = cy / 7.0f;
            if (body_a * body_a + body_b * body_b < 1.2f)
                c = 0xFF8C6B50;

            // Head — smaller circle at front (right side)
            float hx = cx - 5.0f, hy = cy + 1.0f;
            if (hx * hx + hy * hy < 16.0f)
                c = 0xFF8C6B50;

            // Ear
            float ex = cx - 4.0f, ey = cy - 4.0f;
            if (ex * ex + ey * ey < 4.0f)
                c = 0xFFB08C70;

            // Eye
            if (std::abs(cx - 7.0f) < 1.5f && std::abs(cy + 0.0f) < 1.5f)
                c = 0xFF000000;

            // Nose
            if (std::abs(cx - 10.0f) < 1.0f && std::abs(cy + 1.0f) < 1.0f)
                c = 0xFF000000;

            // Tail — thin curve
            float tx = cx + 8.0f, ty = cy - 3.0f;
            if (std::abs(tx) < 5.0f && std::abs(ty) < 1.0f && cx < -6.0f)
                c = 0xFF6B4F38;

            rat_.set_pixel(x, y, c);
        }
    }
}
