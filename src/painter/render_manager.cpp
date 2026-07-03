#include "render_manager.hpp"

#include "graphics/framebuffer.hpp"
#include "graphics/raycaster.hpp"
#include "drawer.hpp"
#include "algorithm/maze.hpp"
#include "entity/entity_thread.hpp"
#include "player/player.hpp"
#include "security/password_overlay.hpp"
#include "ui/minimap.hpp"

RenderManager::RenderManager(Framebuffer& fb, Raycaster& rc,
                             EntityThread& entities, PasswordOverlay& overlay,
                             Drawer& drawer, MazeGenerator& maze, Player& player)
    : fb_(fb), rc_(rc), entities_(entities), overlay_(overlay)
    , drawer_(drawer), maze_(maze), player_(player) {}

void RenderManager::render(GameStateMachine::State state, float wall_height) {
    rc_.cast(maze_, player_.pos_x(), player_.pos_y(),
             player_.dir_x(), player_.dir_y(), wall_height);

    auto& cb = rc_.color_buffer();
    const auto& db = rc_.depth_buffer();
    entities_.render_sprites(cb.data(), db.data(), entities_.camera(),
                             rc_.render_width(), rc_.render_height(),
                             wall_height);

    if (state == GameStateMachine::State::LOCKED)
        overlay_.draw_dots(cb.data(), rc_.render_width(), rc_.render_height());

    drawer_.frame(fb_, rc_, wall_height,
                  state == GameStateMachine::State::LOCKED
                      ? overlay_.overlay_color()
                      : entities_.screen_overlay_color(),
                  state == GameStateMachine::State::LOCKED
                      ? overlay_.overlay_alpha()
                      : entities_.screen_overlay_alpha());

    if (state != GameStateMachine::State::LOCKED)
        draw_minimap(fb_, maze_, entities_.entities(),
                     player_.pos_x(), player_.pos_y(),
                     player_.dir_x(), player_.dir_y(),
                     player_.path());
}
