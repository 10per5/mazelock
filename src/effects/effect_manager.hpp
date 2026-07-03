#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class Effect;
class Framebuffer;

class EffectManager {
public:
    ~EffectManager();

    void init(Framebuffer& fb);
    void shutdown(Framebuffer& fb);

    void set_effect(std::unique_ptr<Effect> effect);
    void clear_effect();

    bool active() const { return effect_ != nullptr; }

    void update(float dt);
    void render(Framebuffer& fb);

private:
    struct PerMonitor {
        int width = 0, height = 0;
        std::unique_ptr<Effect> effect;
    };

    std::unique_ptr<Effect> effect_;
    std::vector<PerMonitor> monitors_;
};
