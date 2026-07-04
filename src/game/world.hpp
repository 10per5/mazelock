#pragma once

#include "game_state_machine.hpp"
#include "ui/input_manager.hpp"
#include "effects/effect_manager.hpp"

#include <memory>

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

    // Owned resources (declared in dependency order for safe destruction)
    std::unique_ptr<Framebuffer> fb_;
    std::unique_ptr<TextureManager> texman_;
    std::unique_ptr<MazeGenerator> maze_;
    std::unique_ptr<Scheduler> scheduler_;
    std::unique_ptr<EntityThread> entities_;
    std::unique_ptr<Raycaster> raycaster_;
    std::unique_ptr<PasswordOverlay> pw_overlay_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<Drawer> drawer_;
    std::unique_ptr<RenderManager> renderer_;
    std::unique_ptr<HeartDisplay> hearts_;
    bool god_mode_ = false;
};
