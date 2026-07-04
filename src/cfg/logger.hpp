#pragma once

#include <cstdarg>
#include <cstdio>

class Logger {
public:
    virtual ~Logger() = default;

    void log(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);
        vwrite(fmt, args);
        va_end(args);
    }

    void debug(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        if (is_active()) {
            va_list args;
            va_start(args, fmt);
            vwrite(fmt, args);
            va_end(args);
        }
    }

protected:
    virtual bool is_active() const = 0;

private:
    static void vwrite(const char* fmt, va_list args) {
        std::vfprintf(stderr, fmt, args);
        std::fputc('\n', stderr);
    }
};

class NullLogger final : public Logger {
protected:
    bool is_active() const override { return false; }
};

class DebugLogger final : public Logger {
protected:
    bool is_active() const override { return true; }
};
