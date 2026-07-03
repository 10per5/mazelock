#include "texture.hpp"

#include <cstdio>
#include <cstdlib>

#ifdef USEPNG
#include <png.h>
#endif

void Texture::create(int w, int h) {
    w_ = w;
    h_ = h;
    pixels_.assign(w * h, 0xFF000000);
}

void Texture::set_pixel(int x, int y, uint32_t c) {
    if (x < 0 || x >= w_ || y < 0 || y >= h_) return;
    pixels_[y * w_ + x] = c;
}

bool Texture::load_png(const std::string& path) {
#ifdef USEPNG
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        std::fprintf(stderr, "Texture: cannot open %s\n", path.c_str());
        return false;
    }

    png_byte sig[8];
    std::fread(sig, 1, 8, fp);
    if (png_sig_cmp(sig, 0, 8) != 0) {
        std::fprintf(stderr, "Texture: %s is not a PNG\n", path.c_str());
        std::fclose(fp);
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                              nullptr, nullptr);
    if (!png) { std::fclose(fp); return false; }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    w_ = png_get_image_width(png, info);
    h_ = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    pixels_.resize(w_ * h_);

    std::vector<png_bytep> rows(h_);
    std::vector<png_byte> raw(w_ * 4 * h_);
    for (int y = 0; y < h_; ++y)
        rows[y] = &raw[y * w_ * 4];

    png_read_image(png, rows.data());
    png_read_end(png, info);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            png_byte* p = &rows[y][x * 4];
            pixels_[y * w_ + x] =
                0xFF000000 |
                (static_cast<uint32_t>(p[0]) << 16) |
                (static_cast<uint32_t>(p[1]) <<  8) |
                (static_cast<uint32_t>(p[2]));
        }
    }

    return true;
#else
    (void)path;
    std::fprintf(stderr, "Texture: PNG support not compiled in\n");
    return false;
#endif
}

uint32_t Texture::pixel(int x, int y) const {
    if (w_ == 0 || h_ == 0) return 0xFF000000;
    x %= w_;
    y %= h_;
    if (x < 0) x += w_;
    if (y < 0) y += h_;
    return pixels_[y * w_ + x];
}
