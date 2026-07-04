#include "entity_thread.hpp"
#include "entity.hpp"
#include "animal.hpp"
#include "coin.hpp"
#include "goal.hpp"

#include "algorithm/maze.hpp"
#include "cfg/config.hpp"
#include "ui/camera.hpp"
#include "graphics/texture_manager.hpp"

#include <cstdio>
#include <cstdlib>

EntityThread::EntityThread() {}
EntityThread::~EntityThread() {}

void EntityThread::set_sprites(const TextureManager& tm) {
    rat_sprite_ = &tm.rat();
    coin_sprite_ = &tm.coin();
    start_sprite_ = &tm.start();
    goal_sprite_ = &tm.goal();
}

void EntityThread::init(MazeGenerator& maze) {
    owned_.clear();
    entities_.clear();

    for (int y = 0; y < maze.height(); ++y) {
        for (int x = 0; x < maze.width(); ++x) {
            auto obj = maze.cell(x, y).object;
            if (obj == OBJECT_COIN)
                owned_.push_back(std::make_unique<Coin>(x, y, coin_sprite_));
            else if (obj == OBJECT_START)
                owned_.push_back(std::make_unique<Goal>(x, y, 0xFF00FFFF, start_sprite_));
            else if (obj == OBJECT_FINISH)
                owned_.push_back(std::make_unique<Goal>(x, y, 0xFFFF0000, goal_sprite_));
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

    for (const auto& e : owned_) {
        if (auto* a = dynamic_cast<Animal*>(e.get()))
            a->update(dt);
    }

    // Coin no longer has per-frame update — just collected on entry

    if (animal_flash_alpha_ > 0.0f) {
        animal_flash_alpha_ -= dt * 2.0f;
        if (animal_flash_alpha_ < 0.0f)
            animal_flash_alpha_ = 0.0f;
    }
}

bool EntityThread::consume_animal_at(int x, int y, MazeGenerator& maze, float player_x, float player_y) {
    for (const auto& e : owned_) {
        auto* a = dynamic_cast<Animal*>(e.get());
        if (a && a->is_active() && a->occupies(x, y)) {
            a->start_flee(player_x, player_y);
            maze.clear_object(x, y);
            animal_flash_alpha_ = 0.5f;
            if (cfg.debug_mode())
                printf("[ANIMAL] consumed at (%d,%d) — fleeing\n", x, y);
            return true;
        }
    }
    return false;
}

bool EntityThread::consume_coin_at(int x, int y, MazeGenerator& maze) {
    for (const auto& e : owned_) {
        auto* c = dynamic_cast<Coin*>(e.get());
        if (c && c->is_active() && c->occupies(x, y)) {
            c->remove();
            maze.clear_object(x, y);
            ++score_;
            if (cfg.debug_mode())
                printf("[COIN] collected at (%d,%d) score=%d\n", x, y, score_);
            return true;
        }
    }
    return false;
}

void EntityThread::render_sprites(uint32_t* color_buffer, const float* depth_buffer,
                                   const Camera& camera,
                                   int render_w, int render_h, float wall_height) const {
    for (const auto& e : owned_) {
        if (auto* a = dynamic_cast<const Animal*>(e.get()))
            a->render(color_buffer, depth_buffer, camera,
                      render_w, render_h, wall_height);
        else if (auto* c = dynamic_cast<const Coin*>(e.get()))
            c->render(color_buffer, depth_buffer, camera,
                      render_w, render_h, wall_height);
        else if (auto* g = dynamic_cast<const Goal*>(e.get()))
            g->render(color_buffer, depth_buffer, camera,
                      render_w, render_h, wall_height);
    }
}

uint32_t EntityThread::screen_overlay_color() const {
    return 0x008800CC;  // purple for animal consume
}

float EntityThread::screen_overlay_alpha() const {
    return animal_flash_alpha_;
}

void EntityThread::reset() {
    owned_.clear();
    entities_.clear();
    animal_flash_alpha_ = 0.0f;
    score_ = 0;
}
