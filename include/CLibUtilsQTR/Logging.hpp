#pragma once

#include <SKSE/SKSE.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <cstddef>
#include <format>
#include <memory>

namespace clib_utilsQTR {
    struct LogOptions {
        /// Rotation limit in bytes per file. Default: 2 MiB.
        std::size_t max_file_size = 2 * 1024 * 1024;
        /// Number of backups besides the current log. Default: 2.
        std::size_t backup_count = 2;
#ifndef NDEBUG
        /// Minimum severity to record. Default: trace in Debug, info otherwise.
        spdlog::level::level_enum level = spdlog::level::trace;
#else
        /// Minimum severity to record. Default: trace in Debug, info otherwise.
        spdlog::level::level_enum level = spdlog::level::info;
#endif
        /// Minimum severity to flush immediately. Defaults to the selected level.
        spdlog::level::level_enum flush_level = level;
    };

    /// Set up logging using the plugin's name and SKSE log directory.
    /// Use SetupLog() for defaults, or customize any fields inline:
    /// @code
    /// clib_utilsQTR::SetupLog({
    ///     .max_file_size = 2 * 1024 * 1024,    // bytes per file
    ///     .backup_count = 2,                  // backups besides the current log
    ///     .level = spdlog::level::info,       // minimum severity to record
    ///     .flush_level = spdlog::level::info  // minimum severity to flush immediately
    /// });
    /// @endcode
    /// Omitted fields keep their defaults; supplied fields must follow this order.
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
