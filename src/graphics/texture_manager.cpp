#include "texture_manager.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "game/singleton.hpp"

bool TextureManager::init(const std::string& tex_dir) {
#ifdef USEPNG
    return load_from_png(tex_dir);
#else
    generate_wall();
    generate_floor();
    generate_ceiling();
    generate_coin();
    generate_rat();
    generate_heart();
    generate_start();
    generate_goal();
    generate_flowers();
    return true;
#endif
}

bool TextureManager::load_from_png(const std::string& dir) {
    bool ok = true;
    auto load = [&](Texture& t, const char* name) {
        std::string path = dir + "/" + name;
        if (!t.load_png(path)) {
            g_logger->log("Failed to load %s", path.c_str());
            ok = false;
        }
    };
    load(wall_,    "wall.png");
    load(floor_,   "floor.png");
    load(ceiling_, "ceiling.png");
    load(coin_,    "coin.png");
    load(rat_,     "rat.png");
    load(heart_,   "heart.png");
    load(start_,   "start.png");
    load(goal_,    "goal.png");

    // Flower art is optional even in PNG mode — fall back to procedural
    // designs per-slot instead of failing the whole load.
    static const char* flower_files[FLOWER_COUNT] = {
        "flower0.png", "flower1.png", "flower2.png", "flower3.png"
    };
    generate_flowers();
    for (int i = 0; i < FLOWER_COUNT; ++i) {
        std::string path = dir + "/" + flower_files[i];
        if (!flowers_[i].load_png(path))
            g_logger->log("No %s found, using procedural flower design", flower_files[i]);
    }

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
            // Smile :)
            if ((x == 13 || x == 19) && y == 19) c = 0xFF000000;
            if ((x == 14 || x == 18) && y == 20) c = 0xFF000000;
            if (x >= 15 && x <= 17 && y == 21) c = 0xFF000000;
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

void TextureManager::generate_heart() {
    int w = 16, h = 16;
    heart_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float fx = (x + 0.5f) / 16.0f * 2.0f - 1.0f;
            float fy = (y + 0.5f) / 16.0f * 2.0f - 1.0f;
            float xx = fx * 1.4f;
            float yy = -fy * 1.3f - 0.1f;
            float x2 = xx * xx;
            float y2 = yy * yy;
            float val = (x2 + y2 - 1.0f) * (x2 + y2 - 1.0f) * (x2 + y2 - 1.0f) - x2 * yy * yy * yy;
            if (val <= 0.0f)
                heart_.set_pixel(x, y, 0xFFFF2040);
            else
                heart_.set_pixel(x, y, 0x00000000);
        }
    }
}

void TextureManager::generate_start() {
    int w = 16, h = 16;
    start_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t c = 0xFF33AA33;
            bool inner = x >= 2 && x < 14 && y >= 2 && y < 14;
            if (!inner) {
                c = 0xFF226622;
            } else {
                bool s_horiz = (y == 5 || y == 8 || y == 11);
                bool s_vert  = ((y >= 3 && y <= 5) || (y >= 8 && y <= 11));
                bool fill = false;
                if (s_horiz && x >= 4 && x <= 11)
                    fill = true;
                else if (s_vert && (x == 4 || x == 11))
                    fill = true;
                else if (y >= 6 && y <= 7 && x == 4)
                    fill = true;
                if (!fill)
                    c = 0xFF33AA33;
                else
                    c = 0xFF006600;
            }
            start_.set_pixel(x, y, c);
        }
    }
}

// ---------------------------------------------------------------------------
// Flower designs — used by FlowerMagnifierEffect. 32x32, alpha 0x00 outside
// the petals so the effect can composite over its own background.
// ---------------------------------------------------------------------------

void TextureManager::generate_flowers() {
    generate_flower_daisy(flowers_[0]);
    generate_flower_tulip(flowers_[1]);
    generate_flower_sunflower(flowers_[2]);
    generate_flower_rose(flowers_[3]);
}

void TextureManager::generate_flower_daisy(Texture& t) {
    int w = 32, h = 32;
    t.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float cx = x - 16.0f + 0.5f;
            float cy = y - 16.0f + 0.5f;
            float dist = std::sqrt(cx * cx + cy * cy);
            float ang = std::atan2(cy, cx);
            uint32_t c = 0x00000000;

            // 8 rounded petals, white with a soft pink tip
            float petal_r = 13.0f + 2.5f * std::cos(ang * 8.0f);
            if (dist < petal_r && dist > 3.5f) {
                float t2 = (dist - 3.5f) / (petal_r - 3.5f);
                int shade = 255 - static_cast<int>(30.0f * t2);
                c = 0xFF000000 | (shade << 16) | ((shade - 10) << 8) | (shade - 5 > 200 ? 220 : 235);
            }
            // Yellow center disc
            if (dist <= 5.0f)
                c = 0xFFFFCC33;
            if (dist <= 5.0f && dist > 3.0f && (static_cast<int>(ang * 6.0f) & 1))
                c = 0xFFE0A81F;

            t.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_flower_tulip(Texture& t) {
    int w = 32, h = 32;
    t.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t c = 0x00000000;
            float fx = (x - 16.0f + 0.5f) / 10.0f;
            float fy = (y - 12.0f + 0.5f) / 13.0f;

            // Cup-shaped bloom: wide at the base (fy ~ 0.6), tapering to a point on top
            float profile = 1.0f - std::abs(fy);
            float width_at_y = profile * (1.0f - 0.35f * std::max(0.0f, -fy));
            if (fy > -1.0f && fy < 1.0f && std::abs(fx) < width_at_y) {
                float shade = 0.75f + 0.25f * (1.0f - std::abs(fx) / (width_at_y + 0.001f));
                int r = static_cast<int>(220 * shade);
                int g = static_cast<int>(40 * shade);
                int b = static_cast<int>(90 * shade);
                c = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
            // Stem
            if (x >= 15 && x <= 16 && y >= 22 && y < 32)
                c = 0xFF339933;
            // Leaf
            if (y >= 24 && y <= 28 && x >= 17 && x <= 22 && (x - 17) <= (28 - y))
                c = 0xFF44AA44;

            t.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_flower_sunflower(Texture& t) {
    int w = 32, h = 32;
    t.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float cx = x - 16.0f + 0.5f;
            float cy = y - 16.0f + 0.5f;
            float dist = std::sqrt(cx * cx + cy * cy);
            float ang = std::atan2(cy, cx);
            uint32_t c = 0x00000000;

            // Long pointed petals
            float petal_r = 15.5f + 1.5f * std::sin(ang * 12.0f);
            if (dist < petal_r && dist > 7.0f) {
                int shade = 210 + static_cast<int>(30.0f * std::cos(ang * 12.0f));
                if (shade > 255) shade = 255;
                c = 0xFF000000 | (shade << 16) | ((shade > 60 ? shade - 60 : 0) << 8) | 0x10;
            }
            // Brown seed disc with a speckled pattern
            if (dist <= 7.5f) {
                bool speckle = ((static_cast<int>(dist * 2.0f) + static_cast<int>(ang * 5.0f)) & 1) != 0;
                c = speckle ? 0xFF5C3A1E : 0xFF7A4E2A;
            }

            t.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_flower_rose(Texture& t) {
    int w = 32, h = 32;
    t.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float cx = x - 16.0f + 0.5f;
            float cy = y - 16.0f + 0.5f;
            float dist = std::sqrt(cx * cx + cy * cy);
            float ang = std::atan2(cy, cx);
            uint32_t c = 0x00000000;

            // Layered spiral petals — a few concentric rings, each slightly
            // rotated, giving a rolled-bloom silhouette instead of flat petals.
            for (int ring = 3; ring >= 0; --ring) {
                float ring_r = 5.0f + ring * 3.0f;
                float wobble = 1.5f * std::cos(ang * 5.0f + ring * 1.1f);
                if (dist < ring_r + wobble) {
                    int base = 150 + ring * 25;
                    int r = std::min(255, base + 40);
                    int g = std::max(0, base - 90);
                    int b = std::max(0, base - 70);
                    c = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }

            t.set_pixel(x, y, c);
        }
    }
}

void TextureManager::generate_goal() {
    int w = 16, h = 16;
    goal_.create(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t c;
            int chk = (x / 2 + y / 2) & 1;
            if (chk)
                c = 0xFFFFFFFF;
            else
                c = 0xFF222222;
            bool border = x < 1 || x >= 15 || y < 1 || y >= 15;
            if (border)
                c = 0xFF444444;
            goal_.set_pixel(x, y, c);
        }
    }
}
