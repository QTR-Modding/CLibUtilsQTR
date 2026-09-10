# Hooks

`write_prologue_hook()` redirects a function entry and returns a trampoline: a callable address reaching the original function body.

```cpp
#include <CLibUtilsQTR/Hooks.hpp>

// Illustrative signature: replace with the target's exact signature.
using TargetFunction = bool (*)(int);
inline TargetFunction original = nullptr;

bool Replacement(int value) {
    return original(value);
}

bool Install(std::uintptr_t targetAddress) {
    const auto trampoline =
        clib_utilsQTR::write_prologue_hook(targetAddress, &Replacement);
    if (!trampoline) return false;
    original = reinterpret_cast<TargetFunction>(trampoline);
    return true;
}
```

Use `hooks` and link Detours:

```cmake
find_library(DETOURS_LIBRARY detours REQUIRED)
target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})
```

The example is an installation pattern, not a Skyrim function signature or address. Resolve the address for your supported runtime and match the actual calling convention and parameters.

Install once, outside another Detours transaction, while other threads cannot execute the target or replacement. The helper enlists only the calling thread; the replacement must not run before `original` is assigned.

Zero means installation failed. The helper does not resolve addresses, manage hook ownership, or provide uninstall.
