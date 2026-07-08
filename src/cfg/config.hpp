#pragma once

#include <string>
#include <string_view>

class Config {
public:
    Config() = default;
    Config(const char* file_path, int argc, char* argv[]);
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) noexcept = default;
    Config& operator=(Config&&) noexcept = default;
    ~Config() = default;

    void print_help() const __attribute__((cold));

    bool        debug_mode()        const { return debug_; }
    bool        minimap()           const { return minimap_; }
    bool        textured_floor()    const { return textured_floor_; }
    float       ai_speed()          const { return ai_speed_; }
    float       floor_tile_scale()  const { return floor_tile_scale_; }
    float       ceiling_tile_scale()const { return ceiling_tile_scale_; }
    int         maze_width()        const { return maze_width_; }
    int         maze_height()       const { return maze_height_; }
    float       wall_grow_time()    const { return wall_grow_time_; }
    float       deep_idle_time()    const { return deep_idle_time_; }
    float       target_fps()        const { return target_fps_; }
    float       idle_fps()          const { return idle_fps_; }
    float       restart_delay()     const { return restart_delay_; }
    bool        quick_fail()        const { return quick_fail_; }
    bool        no_password()       const { return no_password_; }
    bool        help_requested()    const { return help_; }
    std::string_view effect()       const { return effect_; }

private:
    void parse_file(const char* file_path);
    void parse_args(int argc, char* argv[]);

    bool   debug_ = false;
    bool   minimap_ = true;
    bool   textured_floor_ = true;
    float  ai_speed_ = 2.0f;
    float  floor_tile_scale_ = 4.0f;
    float  ceiling_tile_scale_ = 4.0f;
    int    maze_width_ = 15;
    int    maze_height_ = 15;
    float  wall_grow_time_ = 1.0f;
    float  deep_idle_time_ = 300.0f;
    float  target_fps_ = 30.0f;
    float  idle_fps_ = 15.0f;
    float  restart_delay_ = 4.0f;
    bool   quick_fail_ = false;
    bool   no_password_ = false;
    bool   help_ = false;
    std::string effect_;
};

extern Config cfg;
