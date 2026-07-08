#include "entity_thread.hpp"
#include "entity.hpp"
#include "animal.hpp"
#include "coin.hpp"
#include "goal.hpp"

#include "algorithm/maze.hpp"
#include "game/singleton.hpp"
#include "ui/camera.hpp"
#include "graphics/texture_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <vector>

EntityThread::EntityThread() {}
EntityThread::~EntityThread() {}

void EntityThread::set_sprites(const TextureManager& tm) {
    rat_sprite_ = &tm.rat();
    coin_sprite_ = &tm.coin();
    start_sprite_ = &tm.start();
    goal_sprite_ = &tm.goal();
}

void EntityThread::init(MazeGenerator& maze) {
    // Clear raw pointers BEFORE destroying objects to close dangling-pointer window
    entities_.clear();
    owned_.clear();

    for (int y = 0; y < maze.height(); ++y) {
        for (int x = 0; x < maze.width(); ++x) {
            auto obj = maze.cell(x, y).object;
            if (obj == OBJECT_COIN)
                owned_.push_back(std::make_unique<Coin>(x, y, coin_sprite_));
            else if (obj == OBJECT_START)
                owned_.push_back(std::make_unique<Goal>(x, y, 0xFF00FFFF, start_sprite_));
            else if (obj == OBJECT_FINISH)
                owned_.push_back(std::make_unique<Goal>(x, y, 0xFF00FF00, goal_sprite_));
            else if (obj == OBJECT_ANIMAL)
                owned_.push_back(std::make_unique<Animal>(x, y, rat_sprite_));
        }
    }

    for (const auto& e : owned_) entities_.push_back(e.get());
}

void EntityThread::update(float dt, MazeGenerator& maze, int ai_cell_x, int ai_cell_y,
                          float pos_x, float pos_y, float dir_x, float dir_y) {
    camera_.set_position(pos_x, pos_y);
    camera_.set_direction(dir_x, dir_y, 0.66f);

    for (const auto& e : owned_)
        e->update(dt);

    // Coin no longer has per-frame update — just collected on entry

    if (animal_flash_alpha_ > 0.0f) {
        animal_flash_alpha_ -= dt * 2.0f;
        if (animal_flash_alpha_ < 0.0f)
            animal_flash_alpha_ = 0.0f;
    }

    if (coin_flash_alpha_ > 0.0f) {
        coin_flash_alpha_ -= dt * 3.0f;
        if (coin_flash_alpha_ < 0.0f)
            coin_flash_alpha_ = 0.0f;
    }
}

bool EntityThread::consume_animal_at(int x, int y, MazeGenerator& maze, float player_x, float player_y) {
    for (const auto& e : owned_) {
        if (e->try_consume_animal(maze, player_x, player_y, x, y)) {
            maze.clear_object(x, y);
            animal_flash_alpha_ = 0.5f;
            g_logger->debug("[ANIMAL] consumed at (%d,%d) — fleeing", x, y);
            return true;
        }
    }
    return false;
}

bool EntityThread::consume_coin_at(int x, int y, MazeGenerator& maze) {
    for (const auto& e : owned_) {
        if (e->try_consume_coin(maze, x, y)) {
            maze.clear_object(x, y);
            ++score_;
            coin_flash_alpha_ = 0.5f;
            g_logger->debug("[COIN] collected at (%d,%d) score=%d", x, y, score_);
            return true;
        }
    }
    return false;
}

void EntityThread::render_sprites(uint32_t* color_buffer, const float* depth_buffer,
                                   const Camera& camera,
                                   int render_w, int render_h, float wall_height) const {
    float cx = camera.pos_x();
    float cy = camera.pos_y();

    sorted_entries_.clear();
    for (const auto& e : owned_) {
        float dx = e->world_x() - cx;
        float dy = e->world_y() - cy;
        sorted_entries_.push_back({e.get(), dx * dx + dy * dy});
    }

    std::sort(sorted_entries_.begin(), sorted_entries_.end(),
              [](const SortEntry& a, const SortEntry& b) { return a.dist_sq > b.dist_sq; });

    for (const auto& entry : sorted_entries_)
        entry.e->render(color_buffer, depth_buffer, camera,
                        render_w, render_h, wall_height);
}

uint32_t EntityThread::screen_overlay_color() const {
    if (coin_flash_alpha_ > 0.0f) return 0x00FFDD00;  // yellow for coin collect
    return 0x008800CC;  // purple for animal consume
}

float EntityThread::screen_overlay_alpha() const {
    if (coin_flash_alpha_ > 0.0f) return coin_flash_alpha_;
    return animal_flash_alpha_;
}

void EntityThread::reset() {
    owned_.clear();
    entities_.clear();
    animal_flash_alpha_ = 0.0f;
    coin_flash_alpha_ = 0.0f;
    score_ = 0;
}
