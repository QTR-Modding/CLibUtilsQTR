#pragma once

#include <SKSE/SKSE.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <cstddef>
#include <format>
#include <memory>

namespace clib_utilsQTR {
    struct LogOptions {
        std::size_t max_file_size = 2 * 1024 * 1024; // 2 MiB
        std::size_t backup_count = 2;
#ifndef NDEBUG
        spdlog::level::level_enum level = spdlog::level::trace;
#else
        spdlog::level::level_enum level = spdlog::level::info;
#endif
        spdlog::level::level_enum flush_level = level;
    };

    inline void SetupLog(const LogOptions& options = {}) {
        const auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
        const auto logFilePath = *logsFolder / std::format("{}.log", plugin->GetName());
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFilePath.string(), options.max_file_size, options.backup_count, true);
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("log", std::move(sink)));
        spdlog::set_level(options.level);
        spdlog::flush_on(options.flush_level);
        SKSE::log::info("Name of the plugin is {}.", plugin->GetName());
        SKSE::log::info("Version of the plugin is {}.", plugin->GetVersion());
    }
}
