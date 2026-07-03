#include "effect_manager.hpp"
#include "effect.hpp"
#include "graphics/framebuffer.hpp"

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

    // Render to each blackout monitor
    for (size_t i = 0; i < monitors_.size(); ++i) {
        auto* buf = fb.blackout_pixels(static_cast<int>(i));
        if (!buf) continue;
        effect_->render(buf, monitors_[i].width, monitors_[i].height);
        fb.present_blackout(static_cast<int>(i));
    }
}
