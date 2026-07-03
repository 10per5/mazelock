#pragma once

#include "game_state_machine.hpp"
#include "ui/input_manager.hpp"
#include "effects/effect_manager.hpp"

class Config;
class Framebuffer;
class TextureManager;
class MazeGenerator;
class Scheduler;
class EntityThread;
class Raycaster;
class PasswordOverlay;
class Player;
class Drawer;
class RenderManager;
class HeartDisplay;

class World {
public:
    World(Config& cfg, int argc, char* argv[]);
    ~World();
    void run();

private:
    Config& cfg_;
    InputManager input_mgr_;
    GameStateMachine state_machine_;
    EffectManager fx_mgr_;
    RenderManager* renderer_ = nullptr;
    HeartDisplay* hearts_ = nullptr;

    Framebuffer* fb_ = nullptr;
    TextureManager* texman_ = nullptr;
    MazeGenerator* maze_ = nullptr;
    Scheduler* scheduler_ = nullptr;
    EntityThread* entities_ = nullptr;
    Raycaster* raycaster_ = nullptr;
    PasswordOverlay* pw_overlay_ = nullptr;
    Player* player_ = nullptr;
    Drawer* drawer_ = nullptr;
};
