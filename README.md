# CLibUtilsQTR

---

## Installation (via vcpkg)

Add to your `vcpkg.json`:

```json
"dependencies": [
  "clib-utils-qtr"
]
```

In your `CMakeLists.txt`:

```cmake
find_path(ClibUtilsQTR_INCLUDE_DIRS "ClibUtilsQTR/utils.hpp")
target_include_directories(your_target PRIVATE ${ClibUtilsQTR_INCLUDE_DIRS})
```

This is a header-only library. Using [`clib_utilsQTR::write_prologue_hook`](include/CLibUtilsQTR/Hooks.hpp) requires linking Microsoft Detours:

```cmake
find_library(DETOURS_LIBRARY detours REQUIRED)
target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})
```

To use the CLibUtilsQTR port locally, copy the cmake/ folder from the CLibUtilsQTR repository into your project:

```markdown
your-project/
└── cmake/
    └── ports/
        └── clib-utils-qtr/
            ├── portfile.cmake
            └── vcpkg.json

```

## Optional dependencies

All features below are enabled by default, preserving the full-library setup.
To install only hooks, for example:

```json
{
  "name": "clib-utils-qtr",
  "default-features": false,
  "features": ["hooks"]
}
```

Omit `features` to install only the dependency-free base package.

| Feature | Headers | Dependencies |
| --- | --- | --- |
| Base (always available) | `DebugLocks.hpp`, `StringHelpers.hpp`, `Tasker.hpp`, `Ticker.hpp`, `PresetSettings.hpp` | None |
| `skyrim` | Animation, bounding box, debug drawing, forms (including `DynamicFormTracker.hpp`), logging, Papyrus, serialization, and TXT preset helpers | `clib-util` |
| `hooks` | `Hooks.hpp` | `detours` |
| `json` | `PresetHelpers/Config.hpp`, `PresetHelpers/Getters.hpp` | `rapidjson` |
| `yaml-skyrim` | `PresetHelpers/PresetHelpersYAML.hpp` | `yaml-cpp` and the `skyrim` feature |

Include the specific headers you use. `utils.hpp` remains the all-in-one
header and requires all features. Features control dependency installation;
they do not remove headers or change the C++ API. Skyrim helpers still expect
your project's CommonLibSSE/SKSE setup. Link Detours or yaml-cpp when using
their helpers.

## Logging

In your SKSE plugin's load entry point:

```cpp
#include <CLibUtilsQTR/Logging.hpp>

clib_utilsQTR::SetupLog();
```

This uses the plugin declaration's name and the SKSE log directory. Each log
rotates at 2 MiB, retaining two backups, and starts a new file each launch.
Debug builds log and flush at `trace`; release builds log and flush at `info`.
Link your project's CommonLibSSE and spdlog as usual.

Override only the settings you need through `LogOptions`:

```cpp
clib_utilsQTR::SetupLog({.max_file_size = 4 * 1024 * 1024, .backup_count = 1});
```

`level` controls which messages are logged. `flush_level` controls which messages
flush the file buffer immediately and defaults to the selected `level`.

## Debug lock guards

Include `CLibUtilsQTR/DebugLocks.hpp` and give each mutex a distinct tag:

```cpp
#include <CLibUtilsQTR/DebugLocks.hpp>

struct InventoryMutexTag {
    static constexpr auto name = "inventory";
};

std::shared_mutex inventory_mutex;

void UpdateInventory() {
    clib_utilsQTR::DebugUniqueLock<InventoryMutexTag> lock{inventory_mutex};
    // Update the protected inventory.
}
```

Use `DebugSharedLock<Tag>` for shared access. Both guards release the mutex on
destruction and support `unlock()` for early release. They reject same-thread
recursive acquisition, shared/unique conversion while locked, and unlocking
without ownership. Normal contention between threads waits for the mutex.

Use the same tag for every guard of a given mutex, and a different tag for each
other mutex. `DebugLockHeld<Tag>()` reports whether the calling thread holds a
guard with that tag.

To enforce an ordering rule, a tag can provide
`static void CheckLockOrder(const std::source_location& where)`. The guard calls
it before acquiring the mutex. Check other tags with `DebugLockHeld<OtherTag>()`
and call `ReportLockViolation<Tag>("reason", where)` to reject an acquisition.

Violations print the tag's `name`, reason, and source location to standard error
and abort. To use your own logger, provide
`static void ReportViolation(const char* name, const char* reason, const std::source_location& where)`
on the tag. `ReportLockViolation` aborts after that callback returns.

These guards perform their checks regardless of `NDEBUG`. Projects that want
them only in Debug builds can select standard locks in their Release code.
