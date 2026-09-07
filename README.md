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
