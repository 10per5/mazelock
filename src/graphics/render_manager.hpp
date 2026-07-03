#pragma once

#include "game_state_machine.hpp"

class Framebuffer;
class Raycaster;
class EntityThread;
class PasswordOverlay;
class Drawer;
class MazeGenerator;
class Player;

class RenderManager {
public:
    RenderManager(Framebuffer& fb, Raycaster& rc, EntityThread& entities,
                  PasswordOverlay& overlay, Drawer& drawer,
                  MazeGenerator& maze, Player& player);

    void render(GameStateMachine::State state, float wall_height);

private:
    Framebuffer& fb_;
    Raycaster& rc_;
    EntityThread& entities_;
    PasswordOverlay& overlay_;
    Drawer& drawer_;
    MazeGenerator& maze_;
    Player& player_;
};
