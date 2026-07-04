#pragma once

#include <string>

#include "texture.hpp"

class TextureManager {
public:
    bool init(const std::string& tex_dir);

    const Texture& wall()    const { return wall_; }
    const Texture& floor()   const { return floor_; }
    const Texture& ceiling() const { return ceiling_; }
    const Texture& coin()    const { return coin_; }
    const Texture& rat()     const { return rat_; }
    const Texture& heart()   const { return heart_; }
    const Texture& start()   const { return start_; }
    const Texture& goal()    const { return goal_; }

    // Self-declared flower designs, used by FlowerMagnifierEffect.
    static constexpr int FLOWER_COUNT = 4;
    const Texture& flower(int i) const { return flowers_[((i % FLOWER_COUNT) + FLOWER_COUNT) % FLOWER_COUNT]; }
    int flower_count() const { return FLOWER_COUNT; }

private:
    Texture wall_;
    Texture floor_;
    Texture ceiling_;
    Texture coin_;
    Texture rat_;
    Texture heart_;
    Texture start_;
    Texture goal_;
    Texture flowers_[FLOWER_COUNT];

    void generate_wall();
    void generate_floor();
    void generate_ceiling();
    void generate_coin();
    void generate_rat();
    void generate_heart();
    void generate_start();
    void generate_goal();
    void generate_flowers();
    void generate_flower_daisy(Texture& t);
    void generate_flower_tulip(Texture& t);
    void generate_flower_sunflower(Texture& t);
    void generate_flower_rose(Texture& t);

    bool load_from_png(const std::string& dir);
};
