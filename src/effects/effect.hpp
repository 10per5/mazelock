#pragma once

#include <cstdint>

class Effect {
public:
    virtual ~Effect() = default;

    virtual void update(float dt) = 0;
    virtual void render(uint32_t* buffer, int width, int height) = 0;
    virtual const char* name() const = 0;
};
