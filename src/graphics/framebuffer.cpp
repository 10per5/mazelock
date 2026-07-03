#include "framebuffer.hpp"
#include "ui/input_manager.hpp"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <X11/extensions/Xinerama.h>

// ---------------------------------------------------------------------------
// DRM helpers
// ---------------------------------------------------------------------------

std::pair<int, int> Framebuffer::drm_find_connector_crtc(int drm_fd) {
    auto* res = drmModeGetResources(drm_fd);
    if (!res) return {-1, -1};

    int conn_id = -1, crtc_id = -1;
    for (int i = 0; i < res->count_connectors && conn_id < 0; ++i) {
        auto* conn = drmModeGetConnector(drm_fd, res->connectors[i]);
        if (!conn) continue;
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            conn_id = conn->connector_id;
            if (conn->encoder_id) {
                auto* enc = drmModeGetEncoder(drm_fd, conn->encoder_id);
                if (enc) {
                    crtc_id = enc->crtc_id;
                    drmModeFreeEncoder(enc);
                }
            }
            width_  = conn->modes[0].hdisplay;
            height_ = conn->modes[0].vdisplay;
            mode_   = conn->modes[0];
        }
        drmModeFreeConnector(conn);
    }
    drmModeFreeResources(res);
    return {conn_id, crtc_id};
}

bool Framebuffer::try_drm() {
    fd_ = ::open("/dev/dri/card0", O_RDWR);
    if (fd_ < 0) return false;

    auto [conn_id, crtc_id] = drm_find_connector_crtc(fd_);
    if (conn_id < 0 || crtc_id < 0) { ::close(fd_); fd_ = -1; return false; }

    conn_id_ = conn_id;
    crtc_id_ = crtc_id;

    saved_crtc_ = drmModeGetCrtc(fd_, crtc_id);

    struct drm_mode_create_dumb create{};
    create.width  = static_cast<uint32_t>(width_);
    create.height = static_cast<uint32_t>(height_);
    create.bpp    = 32;
    if (::ioctl(fd_, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        cleanup(); return false;
    }

    stride_ = create.pitch;
    dumb_handle_ = create.handle;

    struct drm_mode_map_dumb map{};
    map.handle = create.handle;
    if (::ioctl(fd_, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        cleanup(); return false;
    }

    mapped_ = static_cast<uint8_t*>(
        ::mmap(nullptr, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, map.offset));
    if (mapped_ == MAP_FAILED) { mapped_ = nullptr; cleanup(); return false; }

    if (::drmModeAddFB(fd_, create.width, create.height, 24, 32,
                       create.pitch, create.handle, &fb_id_) < 0) {
        cleanup(); return false;
    }

    if (::drmModeSetCrtc(fd_, crtc_id, fb_id_, 0, 0, &conn_id_, 1, &mode_) < 0) {
        cleanup(); return false;
    }

    backend_ = Backend::drm;
    return true;
}

// ---------------------------------------------------------------------------
// fbdev
// ---------------------------------------------------------------------------

bool Framebuffer::try_fbdev() {
    fd_ = ::open("/dev/fb0", O_RDWR);
    if (fd_ < 0) return false;

    fb_var_screeninfo vinfo;
    fb_fix_screeninfo finfo;
    if (::ioctl(fd_, FBIOGET_VSCREENINFO, &vinfo) < 0) return false;
    if (::ioctl(fd_, FBIOGET_FSCREENINFO, &finfo) < 0)  return false;

    width_  = vinfo.xres;
    height_ = vinfo.yres;
    stride_ = finfo.line_length;

    size_t sz = finfo.line_length * vinfo.yres;
    mapped_ = static_cast<uint8_t*>(
        ::mmap(nullptr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (mapped_ == MAP_FAILED) { mapped_ = nullptr; return false; }

    backend_ = Backend::fbdev;
    return true;
}

// ---------------------------------------------------------------------------
// X11 fullscreen via override_redirect
// ---------------------------------------------------------------------------

bool Framebuffer::try_x11() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) return false;

    int screen = DefaultScreen(display_);
    Window root = RootWindow(display_, screen);

    // Detect the monitor under the mouse cursor via Xinerama
    int win_x = 0, win_y = 0;
    screen_w_ = DisplayWidth(display_, screen);
    screen_h_ = DisplayHeight(display_, screen);

    int root_x, root_y, dummy_x, dummy_y;
    unsigned int mask;
    Window dummy_root, dummy_child;
    XQueryPointer(display_, root, &dummy_root, &dummy_child,
                  &root_x, &root_y, &dummy_x, &dummy_y, &mask);

    int nscreens;
    XineramaScreenInfo* screens = XineramaQueryScreens(display_, &nscreens);
    if (screens) {
        if (XineramaIsActive(display_)) {
            for (int i = 0; i < nscreens; ++i) {
                if (root_x >= screens[i].x_org &&
                    root_x <  screens[i].x_org + screens[i].width &&
                    root_y >= screens[i].y_org &&
                    root_y <  screens[i].y_org + screens[i].height) {
                    win_x = screens[i].x_org;
                    win_y = screens[i].y_org;
                    screen_w_ = screens[i].width;
                    screen_h_ = screens[i].height;
                    break;
                }
            }
        }
    }

    width_  = screen_w_;
    height_ = screen_h_;
    stride_ = width_ * 4;

    XSetWindowAttributes wa{};
    wa.override_redirect = True;
    wa.background_pixel = 0;
    wa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask;

    Visual* visual = DefaultVisual(display_, screen);
    int depth = DefaultDepth(display_, screen);

    window_ = XCreateWindow(display_, root,
                            win_x, win_y, screen_w_, screen_h_, 0,
                            depth, InputOutput, visual,
                            CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);

    XStoreName(display_, window_, "mazelock");
    XMapWindow(display_, window_);

    // Create blackout windows for other monitors
    if (screens && XineramaIsActive(display_)) {
        for (int i = 0; i < nscreens; ++i) {
            if (screens[i].x_org == win_x && screens[i].y_org == win_y &&
                screens[i].width == screen_w_ && screens[i].height == screen_h_)
                continue;
            XSetWindowAttributes bwa{};
            bwa.override_redirect = True;
            bwa.background_pixel = 0;
            bwa.event_mask = ExposureMask;
            Window bw = XCreateWindow(display_, root,
                                      screens[i].x_org, screens[i].y_org,
                                      screens[i].width, screens[i].height, 0,
                                      depth, InputOutput, visual,
                                      CWOverrideRedirect | CWBackPixel | CWEventMask, &bwa);
            if (!bw) continue;

            BlackoutWin bwin;
            bwin.window = bw;
            bwin.width  = screens[i].width;
            bwin.height = screens[i].height;
            bwin.buffer = static_cast<uint8_t*>(std::calloc(1, bwin.width * bwin.height * 4));
            bwin.ximg = XCreateImage(display_, visual, depth, ZPixmap, 0,
                                     nullptr, bwin.width, bwin.height, 32, bwin.width * 4);
            bwin.ximg->bits_per_pixel = 32;
            bwin.ximg->data = reinterpret_cast<char*>(bwin.buffer);
            bwin.gc = XCreateGC(display_, bw, 0, nullptr);
            blackout_wins_.push_back(bwin);
        }
    }

    if (screens)
        XFree(screens);

    ximg_ = XCreateImage(display_, visual, depth, ZPixmap, 0,
                         nullptr, width_, height_, 32, stride_);
    if (!ximg_) {
        XDestroyWindow(display_, window_);
        XCloseDisplay(display_);
        return false;
    }
    ximg_->bits_per_pixel = 32;
    mapped_ = static_cast<uint8_t*>(std::calloc(1, stride_ * height_));
    ximg_->data = reinterpret_cast<char*>(mapped_);

    gc_ = XCreateGC(display_, window_, 0, nullptr);

    XGrabKeyboard(display_, window_, True, GrabModeAsync, GrabModeAsync, CurrentTime);

    // Wait for MapNotify
    XEvent ev;
    do { XNextEvent(display_, &ev); } while (ev.type != MapNotify);

    backend_ = Backend::x11;
    return true;
}

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

Framebuffer::Framebuffer() {
    if (!try_x11()) {
        if (!try_drm()) {
            if (!try_fbdev()) {
                std::exit(1);
            }
        }
    }
}

Framebuffer::~Framebuffer() { cleanup(); }

// ---------------------------------------------------------------------------
// Pixel access
// ---------------------------------------------------------------------------

void Framebuffer::put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    auto* p = mapped_ + y * stride_ + x * 4;
    p[0] =  color        & 0xFF;
    p[1] = (color >> 8)  & 0xFF;
    p[2] = (color >> 16) & 0xFF;
    p[3] = (color >> 24) & 0xFF;
}

uint32_t Framebuffer::get_pixel(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0;
    auto* p = mapped_ + y * stride_ + x * 4;
    return static_cast<uint32_t>(p[0]) |
          (static_cast<uint32_t>(p[1]) << 8) |
          (static_cast<uint32_t>(p[2]) << 16) |
          (static_cast<uint32_t>(p[3]) << 24);
}

// ---------------------------------------------------------------------------
// Bulk row write (hot path)
// ---------------------------------------------------------------------------

void Framebuffer::write_row(int y, const uint32_t* pixels, int count) {
    if (y < 0 || y >= height_ || count <= 0) return;
    if (count > width_) count = width_;
    std::memcpy(mapped_ + y * stride_, pixels, static_cast<size_t>(count) * 4);
}

// ---------------------------------------------------------------------------
// Present — push buffer to screen
// ---------------------------------------------------------------------------

void Framebuffer::present() {
    if (backend_ == Backend::x11 && ximg_) {
        int dx = (screen_w_ - width_) / 2;
        int dy = (screen_h_ - height_) / 2;
        XPutImage(display_, window_, gc_, ximg_, 0, 0, dx, dy, width_, height_);
        XFlush(display_);
    }
}

// ---------------------------------------------------------------------------
// Event loop integration
// ---------------------------------------------------------------------------

bool Framebuffer::handle_events(InputManager& input) {
    if (backend_ == Backend::x11) {
        while (XPending(display_)) {
            XEvent ev;
            XNextEvent(display_, &ev);

            switch (ev.type) {
            case KeyPress: {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                input.on_event(ks, true);
                break;
            }
            case KeyRelease: {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                input.on_event(ks, false);
                break;
            }
            case Expose:
                present();
                break;
            default:
                break;
            }
        }
        return true;
    }

    // DRM / fbdev — read stdin for ESC
    char ch = 0;
    while (::read(STDIN_FILENO, &ch, 1) > 0) {
        if (ch == 27) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Multi-monitor blackout
// ---------------------------------------------------------------------------

void Framebuffer::blackout_other_windows(bool enable) {
    if (backend_ != Backend::x11) return;
    if (blackout_wins_.empty()) return;

    for (auto& bw : blackout_wins_) {
        if (enable) {
            XMapWindow(display_, bw.window);
            XRaiseWindow(display_, bw.window);
            present_blackout(static_cast<int>(&bw - blackout_wins_.data()));
        } else {
            XUnmapWindow(display_, bw.window);
        }
    }
    XFlush(display_);
}

uint32_t* Framebuffer::blackout_pixels(int idx) {
    return reinterpret_cast<uint32_t*>(blackout_wins_[idx].buffer);
}

void Framebuffer::present_blackout(int idx) {
    auto& bw = blackout_wins_[idx];
    if (backend_ == Backend::x11)
        XPutImage(display_, bw.window, bw.gc, bw.ximg, 0, 0, 0, 0, bw.width, bw.height);
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

void Framebuffer::cleanup() {
    if (backend_ == Backend::x11) {
        XUngrabKeyboard(display_, CurrentTime);
        for (auto& bw : blackout_wins_) {
            if (bw.ximg) {
                bw.ximg->data = nullptr;
                XDestroyImage(bw.ximg);
            }
            std::free(bw.buffer);
            if (bw.gc) XFreeGC(display_, bw.gc);
            XDestroyWindow(display_, bw.window);
        }
        blackout_wins_.clear();
        if (ximg_) {
            ximg_->data = nullptr;
            XDestroyImage(ximg_);
        }
        std::free(mapped_);
        mapped_ = nullptr;
        if (gc_)   XFreeGC(display_, gc_);
        if (window_) XDestroyWindow(display_, window_);
        if (display_) XCloseDisplay(display_);
        return;
    }

    if (saved_crtc_ && fd_ >= 0) {
        ::drmModeSetCrtc(fd_, saved_crtc_->crtc_id, saved_crtc_->buffer_id,
                         saved_crtc_->x, saved_crtc_->y,
                         &conn_id_, 1, &saved_crtc_->mode);
    }
    if (fb_id_ && fd_ >= 0) ::drmModeRmFB(fd_, fb_id_);
    if (dumb_handle_ && fd_ >= 0) {
        struct drm_mode_destroy_dumb destroy{};
        destroy.handle = dumb_handle_;
        ::ioctl(fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
    if (mapped_) ::munmap(mapped_, static_cast<size_t>(height_ * stride_));
    if (fd_ >= 0) ::close(fd_);
    if (saved_crtc_) ::drmModeFreeCrtc(saved_crtc_);
    mapped_ = nullptr;
    fd_ = -1;
    fb_id_ = 0;
    dumb_handle_ = 0;
}
