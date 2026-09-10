# Logging

After initializing SKSE in your plugin's load function, call:

```cpp
#include <CLibUtilsQTR/Logging.hpp>

// Inside plugin initialization:
clib_utilsQTR::SetupLog();
SKSE::log::info("Plugin initialized");
```

`SetupLog()` installs the default spdlog logger. It gets the filename and version from your `SKSE::PluginDeclaration` and writes to SKSE's log directory. Use the `skyrim` feature.

## Customize defaults

Pass only the fields you want to change:

```cpp
clib_utilsQTR::SetupLog({ .backup_count = 1 });
```

These are all available fields, in declaration order:

```cpp
clib_utilsQTR::SetupLog({
    .max_file_size = 2 * 1024 * 1024,    // Rotation threshold: bytes per file
    .backup_count = 2,                  // Backups besides the current log
    .level = spdlog::level::info,        // Minimum severity recorded
    .flush_level = spdlog::level::info   // Minimum severity flushed immediately
});
```

C++ designated initializers must follow that order. No separate options object is needed.

| Setting | Default |
| --- | --- |
| Rotation threshold | 2 MiB per file |
| Backups | 2, besides the current file |
| Recording level | `trace` without `NDEBUG`; `info` with `NDEBUG` |
| Flush level | The selected recording level |

For `MyPlugin`, the files are `MyPlugin.log`, `MyPlugin.1.log`, and `MyPlugin.2.log`. Rotation happens on opening a new session and when the size threshold is reached.

Recording and flushing are separate choices. A recorded message can remain buffered until a flush; a message below the recording level is discarded. Use `trace` for routine diagnostic detail and `info` for messages worth retaining in normal play.
