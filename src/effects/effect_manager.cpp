#include "effect_manager.hpp"
#include "effect.hpp"
#include "graphics/framebuffer.hpp"

#include <cstring>

EffectManager::~EffectManager() = default;

void EffectManager::init(Framebuffer& fb) {
    size_t n = fb.num_blackouts();
    monitors_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        monitors_[i].width  = fb.blackout_width(static_cast<int>(i));
        monitors_[i].height = fb.blackout_height(static_cast<int>(i));
    }
}

void EffectManager::shutdown(Framebuffer& /*fb*/) {
    monitors_.clear();
    effect_.reset();
}

void EffectManager::set_effect(std::unique_ptr<Effect> effect) {
    effect_ = std::move(effect);
    // Clone the effect for each monitor (each gets its own state)
    // For now, each monitor shares the same effect instance.
    // A more sophisticated approach would deep-copy per monitor.
}

void EffectManager::clear_effect() {
    effect_.reset();
}

void EffectManager::update(float dt) {
    if (!effect_) return;
    effect_->update(dt);
}

void EffectManager::render(Framebuffer& fb) {
    if (!effect_) return;

    // Most multi-monitor rigs have at least one pair of identical panels.
    // Render once per distinct (width, height) and memcpy the finished
    // frame to any other monitor that matches, instead of re-running the
    // whole effect (which is real CPU work, not just the final blit).
    int last_w = -1, last_h = -1;
    const uint32_t* last_buf = nullptr;

    for (size_t i = 0; i < monitors_.size(); ++i) {
        auto* buf = fb.blackout_pixels(static_cast<int>(i));
        if (!buf) continue;

        int w = monitors_[i].width;
        int h = monitors_[i].height;

        if (last_buf && w == last_w && h == last_h) {
            std::memcpy(buf, last_buf, static_cast<size_t>(w) * h * sizeof(uint32_t));
        } else {
            effect_->render(buf, w, h);
        }

        last_buf = buf;
        last_w = w;
        last_h = h;
    }
}
