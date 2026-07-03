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

private:
    Texture wall_;
    Texture floor_;
    Texture ceiling_;
    Texture coin_;
    Texture rat_;
    Texture heart_;

    void generate_wall();
    void generate_floor();
    void generate_ceiling();
    void generate_coin();
    void generate_rat();
    void generate_heart();

    bool load_from_png(const std::string& dir);
};
