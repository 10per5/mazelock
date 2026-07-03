#include "world.hpp"
#include "config.hpp"
#include "terminal.hpp"

#include "player/player.hpp"
#include "algorithm/scheduler.hpp"
#include "entity/entity_thread.hpp"
#include "painter/drawer.hpp"
#include "graphics/framebuffer.hpp"
#include "painter/render_manager.hpp"
#include "graphics/texture_manager.hpp"
#include "algorithm/maze.hpp"
#include "graphics/raycaster.hpp"
#include "security/password_overlay.hpp"
#include "effects/pipe_effect.hpp"

#include <chrono>
#include <cstdio>
#include <unistd.h>

World::World(Config& cfg, int argc, char* argv[])
    : cfg_(cfg) {
    (void)argc; (void)argv;
    fb_ = new Framebuffer;
    fb_->blackout_other_windows(true);
    fx_mgr_.init(*fb_);

    texman_ = new TextureManager;
    texman_->init("textures");

    maze_ = new MazeGenerator(cfg.maze_width(), cfg.maze_height());
    scheduler_ = new Scheduler;
    entities_ = new EntityThread;
    raycaster_ = new Raycaster;
    pw_overlay_ = new PasswordOverlay;

    raycaster_->set_textures(*texman_);
    entities_->set_sprites(*texman_);
    entities_->init(*maze_);

    if (cfg.debug_mode()) {
        printf("Maze: %dx%d  Start=(0,0)  Finish=(%d,%d)\n",
               maze_->width(), maze_->height(),
               maze_->width() - 1, maze_->height() - 1);
        maze_->print();
    }

    player_ = new Player;
    player_->init(*maze_, *entities_);
    drawer_ = new Drawer;

    renderer_ = new RenderManager(*fb_, *raycaster_, *entities_,
                                   *pw_overlay_, *drawer_, *maze_, *player_);
}

World::~World() {
    fx_mgr_.shutdown(*fb_);
    delete renderer_;
    delete drawer_;
    delete player_;
    delete pw_overlay_;
    delete raycaster_;
    delete entities_;
    delete scheduler_;
    delete maze_;
    delete texman_;
    delete fb_;
}

void World::run() {
    TerminalGuard tguard;

    auto last_time = std::chrono::high_resolution_clock::now();
    auto idle_start = last_time;

    bool running = true;

    // Pipe screensaver runs on secondary monitors at all times
    fx_mgr_.set_effect(std::make_unique<PipeEffect>());

    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        // ---- Event polling ----
        if (!fb_->handle_events(input_mgr_)) break;

        // ---- Key queue processing ----
        {
            KeySym press_ks;
            while ((press_ks = input_mgr_.consume_press()) != NoSymbol) {
                auto st = state_machine_.state();
                if (st == GameStateMachine::State::RUNNING &&
                    press_ks >= XK_space && press_ks <= XK_asciitilde) {
                    state_machine_.lock();
                    pw_overlay_->activate();
                    pw_overlay_->handle_key(press_ks);
                    player_->enable_autowalk();
                    input_mgr_.reset_state();
                    idle_start = now;
                } else if (st == GameStateMachine::State::LOCKED) {
                    pw_overlay_->handle_key(press_ks);
                    idle_start = now;
                }
            }
        }

        // ---- F11 combo handling ----
        if (state_machine_.state() != GameStateMachine::State::LOCKED) {
            const auto& keys = input_mgr_.state();

            if (keys.f11_combo >= 3) {
                player_->pathfind_to_finish();
                input_mgr_.reset_f11_combo();
            }

            static bool f11_edge = false;
            if (keys.f11_combo == 1 && !f11_edge) {
                f11_edge = true;
                player_->enable_autowalk();
            } else if (keys.f11_combo == 0) {
                f11_edge = false;
            }
        }

        // ---- Force quit (gated on --debug) ----
        if (input_mgr_.state().quit_requested && cfg_.debug_mode()) {
            running = false;
            break;
        }

        // ---- Pipe screensaver (always running on secondary monitors) ----
        if (fb_->num_blackouts() > 0) {
            fx_mgr_.update(dt);
            fx_mgr_.render(*fb_);
        }

        // ---- LOCKED state update (password overlay) ----
        if (state_machine_.state() == GameStateMachine::State::LOCKED) {
            pw_overlay_->update(dt);

            if (pw_overlay_->exit_requested()) {
                running = false;
                break;
            }
            if (!pw_overlay_->active()) {
                state_machine_.unlock();
                input_mgr_.reset_state();
            }
        }

        // ---- FPS capping ----
        auto idle_sec = std::chrono::duration<float>(now - idle_start).count();
        bool deep_idle = (idle_sec > cfg_.deep_idle_time() &&
                          state_machine_.state() != GameStateMachine::State::LOCKED);
        float target_fps = deep_idle ? cfg_.idle_fps() : cfg_.target_fps();
        float frame_time = 1.0f / target_fps;
        if (dt < frame_time) {
            ::usleep(static_cast<useconds_t>((frame_time - dt) * 1'000'000));
            dt = frame_time;
        }

        // ---- Arrow key processing ----
        {
            const auto& keys = input_mgr_.state();

            static bool prev_up = false, prev_down = false;
            static bool prev_left = false, prev_right = false;

            if (state_machine_.state() != GameStateMachine::State::LOCKED) {
                bool edge_up    = keys.up    && !prev_up;
                bool edge_down  = keys.down  && !prev_down;
                bool edge_left  = keys.left  && !prev_left;
                bool edge_right = keys.right && !prev_right;

                if (edge_up)    player_->move_forward();
                if (edge_down)  player_->move_back();
                if (edge_left)  player_->turn_left();
                if (edge_right) player_->turn_right();

                int held = 0;
                if (keys.up)        held = 1;
                else if (keys.down) held = 2;
                else if (keys.left) held = 3;
                else if (keys.right)held = 4;
                player_->set_held_dir(held);

                player_->set_speed_mult(keys.shift ? 3.0f : 1.0f);
            }

            prev_up    = keys.up;
            prev_down  = keys.down;
            prev_left  = keys.left;
            prev_right = keys.right;
        }

        // ---- Game state / simulation update ----
        state_machine_.update(dt, *player_, *scheduler_);

        if (state_machine_.consume_regenerate_flag()) {
            maze_->regenerate();
            entities_->init(*maze_);
            player_->reset();
        }

        // ---- Rendering ----
        renderer_->render(state_machine_.state(), state_machine_.wall_height());
    }

    fb_->blackout_other_windows(false);
}
