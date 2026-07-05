#include "config.hpp"

#include <charconv>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

Config cfg;

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Config::Config(const char* file_path, int argc, char* argv[]) {
    parse_file(file_path);
    parse_args(argc, argv);
}

static bool str_to_bool(std::string_view s) {
    return !(s == "0" || s == "false" || s == "False" || s == "FALSE");
}

void Config::parse_file(const char* file_path) {
    std::ifstream f(file_path);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key.empty()) continue;

        if (key == "debug")                    debug_ = str_to_bool(val);
        else if (key == "minimap")             minimap_ = str_to_bool(val);
        else if (key == "textured_floor")      textured_floor_ = str_to_bool(val);
        else if (key == "ai_speed") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                ai_speed_ = v;
        }
        else if (key == "floor_tile_scale") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                floor_tile_scale_ = v;
        }
        else if (key == "ceiling_tile_scale") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                ceiling_tile_scale_ = v;
        }
        else if (key == "maze_width") {
            int v = 0;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                maze_width_ = v;
        }
        else if (key == "maze_height") {
            int v = 0;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                maze_height_ = v;
        }
        else if (key == "wall_grow_time") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                wall_grow_time_ = v;
        }
        else if (key == "deep_idle_time") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                deep_idle_time_ = v;
        }
        else if (key == "target_fps") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                target_fps_ = v;
        }
        else if (key == "idle_fps") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                idle_fps_ = v;
        }
        else if (key == "restart_delay") {
            float v = 0.0f;
            if (std::from_chars(val.data(), val.data() + val.size(), v).ec == std::errc{})
                restart_delay_ = v;
        }
        else if (key == "quick_fail")         quick_fail_ = str_to_bool(val);
        else if (key == "effect")             effect_ = val;
    }
}

void Config::parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0)
            debug_ = true;
        else if (std::strcmp(argv[i], "--quick-fail") == 0)
            quick_fail_ = true;
        else if (std::strcmp(argv[i], "--effect") == 0) {
            if (i + 1 < argc) {
                effect_ = argv[++i];
            }
        }
    }
}
