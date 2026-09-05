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

This is a header-only library. Using the prologue hook helper additionally requires linking Microsoft Detours (see below).

To use the CLibUtilsQTR port locally, copy the cmake/ folder from the CLibUtilsQTR repository into your project:

```markdown
your-project/
└── cmake/
    └── ports/
        └── clib-utils-qtr/
            ├── portfile.cmake
            └── vcpkg.json

```

## Prologue hooks

```cpp
#include <CLibUtilsQTR/Hooks.hpp>

struct Hook {
    static inline int (*original)(int) = nullptr;

    static int thunk(int value) {
        return original(value);
    }

    static bool Install(std::uintptr_t address) {
        original = reinterpret_cast<decltype(original)>(
            clib_utilsQTR::write_prologue_hook(address, thunk));
        return original != nullptr;
    }
};
```

`write_prologue_hook` redirects calls at a function's entry to your replacement and returns a trampoline: a callable address that runs the original function. It returns `0` if installation fails. The target and replacement must have identical signatures and calling conventions.

Install once, while no other thread can execute the target or replacement, and store the returned address before allowing calls. The helper manages its own Detours transaction and enlists only the current thread; do not call it inside another transaction. Keep the replacement loaded for the lifetime of the hook.

Detours is included in the vcpkg dependencies. Add its library to your target:

```cmake
find_library(DETOURS_LIBRARY detours REQUIRED)
target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})
```

The helper is also available through `CLibUtilsQTR/utils.hpp`. Based on the [Particle-Wind helper](https://github.com/RavenKZP/Particle-Wind/blob/bdc66a7415149ee65b1aa87924d0612d6afd7881/include/detour_stl.h#L7).
