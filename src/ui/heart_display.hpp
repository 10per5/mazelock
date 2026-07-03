#pragma once

class Framebuffer;
class PasswordOverlay;
class TextureManager;

class HeartDisplay {
public:
    HeartDisplay(PasswordOverlay& overlay, TextureManager& texman);

    void draw(Framebuffer& fb) const;

private:
    PasswordOverlay& overlay_;
    TextureManager& texman_;

    static constexpr int TEX = 16;
};
