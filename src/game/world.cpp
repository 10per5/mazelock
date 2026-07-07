#include "world.hpp"
#include "cfg/config.hpp"
#include "game/singleton.hpp"
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
#include "ui/heart_display.hpp"
#include "effects/pipe_effect.hpp"
#include "effects/fish_aquarium.hpp"
#include "effects/flower_magnifier.hpp"

#include <chrono>
#include <random>
#include <string_view>

#include <unistd.h>

World::~World() = default;

World::World(Config& cfg, int argc, char* argv[])
    : cfg_(cfg)
    , fb_(std::make_unique<Framebuffer>())
    , texman_(std::make_unique<TextureManager>())
    , maze_(std::make_unique<MazeGenerator>(cfg.maze_width(), cfg.maze_height()))
    , scheduler_(std::make_unique<Scheduler>())
    , entities_(std::make_unique<EntityThread>())
    , raycaster_(std::make_unique<Raycaster>())
    , pw_overlay_(std::make_unique<PasswordOverlay>())
    , player_(std::make_unique<Player>())
    , drawer_(std::make_unique<Drawer>()) {
    (void)argc; (void)argv;
    g_logger = cfg.debug_mode()
        ? static_cast<Logger*>(new DebugLogger)
        : static_cast<Logger*>(new NullLogger);
    fb_->blackout_other_windows(true);
    fx_mgr_.init(*fb_);

    texman_->init("textures");
    raycaster_->set_textures(*texman_);
    entities_->set_sprites(*texman_);
    entities_->init(*maze_);

    if (cfg.debug_mode()) {
        g_logger->debug("Maze: %dx%d  Start=(0,0)  Finish=(%d,%d)",
               maze_->width(), maze_->height(),
               maze_->width() - 1, maze_->height() - 1);
        maze_->print();
    }

    player_->init(*maze_, *entities_);

    renderer_ = std::make_unique<RenderManager>(*fb_, *raycaster_, *entities_,
                                                  *pw_overlay_, *drawer_,
                                                  *maze_, *player_);
    hearts_ = std::make_unique<HeartDisplay>(*pw_overlay_, *texman_);
}



void World::run() {
    TerminalGuard tguard;

    auto last_time = std::chrono::high_resolution_clock::now();
    auto idle_start = last_time;

    bool running = true;

    // A screensaver runs on secondary monitors at all times — selected
    // via config file or --effect CLI flag, or random if unspecified.
    {
        std::string_view fx = cfg_.effect();
        if (!fx.empty()) {
            if (fx == "none") {
            } else if (fx == "pipe") {
                fx_mgr_.set_effect(std::make_unique<PipeEffect>());
            } else if (fx == "fish") {
                fx_mgr_.set_effect(std::make_unique<FishAquariumEffect>());
            } else if (fx == "flower") {
                fx_mgr_.set_effect(std::make_unique<FlowerMagnifierEffect>(*texman_));
            } else {
                g_logger->log("[CONFIG] unknown --effect '%s', ignoring (showing random or none)",
                              std::string(fx).c_str());
            }
        } else {
            std::mt19937 effect_rng{std::random_device{}()};
            int choice = std::uniform_int_distribution<int>(0, 2)(effect_rng);
            switch (choice) {
                case 0:  fx_mgr_.set_effect(std::make_unique<PipeEffect>()); break;
                case 1:  fx_mgr_.set_effect(std::make_unique<FishAquariumEffect>()); break;
                default: fx_mgr_.set_effect(std::make_unique<FlowerMagnifierEffect>(*texman_)); break;
            }
        }
    }

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
                    ((press_ks >= XK_space && press_ks <= XK_asciitilde) || press_ks == XK_Escape)) {
                    if (cfg_.no_password()) {
                        running = false;
                        break;
                    }
                    state_machine_.lock();
                    lock_escape_cooldown_ = 30; // ~500ms at 60fps
                    pw_overlay_->activate();
                    if (press_ks != XK_Escape)
                        pw_overlay_->handle_key(press_ks);
                    player_->enable_autowalk();
                    idle_start = now;
                } else if (st == GameStateMachine::State::LOCKED) {
                    // Debounce Escape via frame cooldown — key repeat
                    // fires ~30ms apart, so ignore for ~500ms
                    if (press_ks != XK_Escape || lock_escape_cooldown_ <= 0)
                        pw_overlay_->handle_key(press_ks);
                    idle_start = now;
                }
            }
        }

        // ---- F10/F11 combo handling ----
        if (state_machine_.state() != GameStateMachine::State::LOCKED) {
            const auto& keys = input_mgr_.state();

            if (keys.f10_combo >= 3) {
                god_mode_ = !god_mode_;
                player_->set_god_mode(god_mode_);
                input_mgr_.reset_f10_combo();
                g_logger->debug("[WORLD] god_mode=%d", god_mode_);
            }

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

        // ---- F1 — reset to start ----
        {
            const auto& keys = input_mgr_.state();
            static bool f1_edge = false;
            if (keys.f1_pressed && !f1_edge) {
                f1_edge = true;
                player_->reset();
                entities_->init(*maze_);
                g_logger->debug("[WORLD] F1 — reset to start");
            } else if (!keys.f1_pressed) {
                f1_edge = false;
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

                player_->set_speed_mult(keys.tab ? 3.0f : 1.0f);
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
            pw_overlay_->reset_bad_attempts();
        }

        // ---- Rendering ----
        renderer_->render(state_machine_.state(), state_machine_.wall_height());
        hearts_->draw(*fb_);
        fb_->present();

        // ---- Blackout windows: present with overlay if LOCKED ----
        for (size_t i = 0; i < fb_->num_blackouts(); ++i) {
            auto* buf = fb_->blackout_pixels(static_cast<int>(i));
            int w = fb_->blackout_width(static_cast<int>(i));
            int h = fb_->blackout_height(static_cast<int>(i));
            if (buf && w > 0 && h > 0) {
                if (state_machine_.state() == GameStateMachine::State::LOCKED)
                    pw_overlay_->apply_overlay(buf, w, h);
                fb_->present_blackout(static_cast<int>(i));
            }
        }

        if (lock_escape_cooldown_ > 0)
            --lock_escape_cooldown_;
    }

    fb_->blackout_other_windows(false);
}
