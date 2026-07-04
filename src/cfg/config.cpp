#include "config.hpp"

#include <charconv>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

Config cfg;

struct Config::Impl {
    std::map<std::string, std::string> map;
    mutable std::map<std::string, int> int_cache;
};

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Config::Config(const char* file_path, int argc, char* argv[])
    : self_(std::make_unique<Impl>()) {
    std::ifstream f(file_path);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));
            if (!key.empty()) self_->map[key] = val;
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0)
            set_int("debug", 1);
        if (std::strcmp(argv[i], "--quick-fail") == 0)
            set_int("quick_fail", 1);
    }
}

Config::~Config() = default;

Config::Config(Config&& other) noexcept = default;
Config& Config::operator=(Config&& other) noexcept = default;

int Config::get_int(const std::string& key, int def) const {
    if (!self_) return def;
    auto ic = self_->int_cache.find(key);
    if (ic != self_->int_cache.end()) return ic->second;
    auto it = self_->map.find(key);
    if (it != self_->map.end()) {
        int v = 0;
        std::from_chars(it->second.data(), it->second.data() + it->second.size(), v);
        self_->int_cache[key] = v;
        return v;
    }
    return def;
}

float Config::get_float(const std::string& key, float def) const {
    if (!self_) return def;
    auto it = self_->map.find(key);
    if (it != self_->map.end()) {
        float v = 0.0f;
        std::from_chars(it->second.data(), it->second.data() + it->second.size(), v);
        return v;
    }
    return def;
}

void Config::set_int(const std::string& key, int value) {
    if (!self_) self_ = std::make_unique<Impl>();
    self_->map[key] = std::to_string(value);
    self_->int_cache[key] = value;
}
