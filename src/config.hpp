#pragma once

#include <string>

class Config {
public:
    Config() = default;
    Config(const char* file_path, int argc, char* argv[]);
    Config(Config&& other) noexcept;
    Config& operator=(Config&& other) noexcept;
    ~Config();

    int get_int(const std::string& key, int def) const;
    float get_float(const std::string& key, float def) const;
    void set_int(const std::string& key, int value);

    // Named accessors for well-known keys
    bool   debug_mode()       const { return get_int("debug", 0) != 0; }
    bool   minimap()          const { return get_int("minimap", 1) != 0; }
    bool   textured_floor()   const { return get_int("textured_floor", 1) != 0; }
    float  ai_speed()         const { return get_float("ai_speed", 2.0f); }
    float  floor_tile_scale() const { return get_float("floor_tile_scale", 4.0f); }
    float  ceiling_tile_scale() const { return get_float("ceiling_tile_scale", 4.0f); }
    int    maze_width()       const { return get_int("maze_width", 15); }
    int    maze_height()      const { return get_int("maze_height", 15); }
    float  wall_grow_time()   const { return get_float("wall_grow_time", 1.0f); }
    float  deep_idle_time()   const { return get_float("deep_idle_time", 300.0f); }
    float  target_fps()       const { return get_float("target_fps", 30.0f); }
    float  idle_fps()         const { return get_float("idle_fps", 15.0f); }
    float  restart_delay()    const { return get_float("restart_delay", 4.0f); }

private:
    struct Impl;
    Impl* self_ = nullptr;
};

extern Config cfg;
