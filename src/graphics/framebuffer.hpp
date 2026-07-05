#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

class InputManager;

class Framebuffer {
    enum class Backend { none, x11, drm, fbdev };
    Backend backend_{Backend::none};

    // Shared members
    int width_ = 0;
    int height_ = 0;
    int stride_ = 0;
    uint8_t* mapped_ = nullptr;

    // DRM / fbdev members
    int fd_ = -1;
    uint32_t fb_id_ = 0;
    uint32_t conn_id_ = 0;
    uint32_t crtc_id_ = 0;
    uint32_t dumb_handle_ = 0;
    drmModeCrtcPtr saved_crtc_ = nullptr;
    drmModeModeInfo mode_{};

    // X11 members
    Display* display_ = nullptr;
    Window window_ = 0;
    GC gc_ = nullptr;
    XImage* ximg_ = nullptr;
    bool use_shm_ = false;
    XShmSegmentInfo shminfo_{};
    int screen_w_ = 0;
    int screen_h_ = 0;

    struct BlackoutWin {
        Window window = 0;
        int width = 0, height = 0;
        uint8_t* buffer = nullptr;
        XImage* ximg = nullptr;
        XShmSegmentInfo shminfo{};
        GC gc = nullptr;
    };
    std::vector<BlackoutWin> blackout_wins_;

    std::pair<int, int> drm_find_connector_crtc(int drm_fd);
    bool try_drm();
    bool try_fbdev();
    bool try_x11();
    void cleanup();

public:
    Framebuffer();
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) = delete;
    Framebuffer& operator=(Framebuffer&&) = delete;

    int xres() const { return width_; }
    int yres() const { return height_; }
    int stride() const { return stride_; }
    int bpp()  const { return 32; }

    std::span<uint8_t> screen() { return {mapped_, static_cast<size_t>(height_ * stride_)}; }

    void put_pixel(int x, int y, uint32_t color);
    uint32_t get_pixel(int x, int y) const;
    void write_row(int y, const uint32_t* pixels, int count);
    void present();
    bool handle_events(InputManager& input);
    void blackout_other_windows(bool enable);

    // Multi-monitor blackout rendering
    size_t num_blackouts() const { return blackout_wins_.size(); }
    int  blackout_width(int idx)  const { return blackout_wins_[idx].width; }
    int  blackout_height(int idx) const { return blackout_wins_[idx].height; }
    uint32_t* blackout_pixels(int idx);
    void present_blackout(int idx);
};