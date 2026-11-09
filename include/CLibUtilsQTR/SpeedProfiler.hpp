#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <spdlog/spdlog.h>

namespace clib_utilsQTR {
    class SpeedProfiler {
    public:
        explicit SpeedProfiler(std::string_view name);
        ~SpeedProfiler();

        SpeedProfiler(const SpeedProfiler&) = delete;
        SpeedProfiler& operator=(const SpeedProfiler&) = delete;

    private:
        std::string name;
        std::chrono::steady_clock::time_point start;
    };

    inline SpeedProfiler::SpeedProfiler(std::string_view name)
        : name(name), start(std::chrono::steady_clock::now()) {}

    inline SpeedProfiler::~SpeedProfiler() {
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        spdlog::info("{}: Elapsed time: {}", name, elapsed.count());
    }
}
