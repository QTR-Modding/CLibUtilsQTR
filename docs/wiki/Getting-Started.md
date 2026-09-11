# Getting started

Use one header for the operation you need. This example trims whitespace without Skyrim dependencies:

```cpp
#include <CLibUtilsQTR/StringHelpers.hpp>
const auto name = StringHelpers::trim("  My preset  ");
// name == "My preset"
```

CLibUtilsQTR is a header-only C++23 library. These guides cover the code on `main`.

## Install with vcpkg

1. Copy `cmake/ports/clib-utils-qtr` from this repository into your project's `cmake/ports/clib-utils-qtr`.
2. Add the overlay to your `vcpkg-configuration.json`:

   ```json
   { "overlay-ports": ["cmake/ports"] }
   ```

3. Add a dependency to your `vcpkg.json`. This example selects the Skyrim helpers:

   ```json
   {
     "dependencies": [
       { "name": "clib-utils-qtr", "default-features": false, "features": ["skyrim"] }
     ]
   }
   ```

   Merge these entries into existing files, keeping your project's other configuration.

4. Add the include directory to your existing CMake target:

   ```cmake
   find_path(ClibUtilsQTR_INCLUDE_DIRS "CLibUtilsQTR/StringHelpers.hpp" REQUIRED)
   target_include_directories(your_target PRIVATE ${ClibUtilsQTR_INCLUDE_DIRS})
   target_compile_features(your_target PRIVATE cxx_std_23)
   ```

The overlay pins a source revision. Copy both port files together when updating.

## Choose dependencies

A vcpkg feature selects external dependencies. All headers are installed regardless of feature selection; include only headers whose dependencies your project provides.

| Feature | Helpers | Dependencies installed |
| --- | --- | --- |
| Base package | Strings, debug locks, Tasker, Ticker, preset values, DLL signing | None |
| `skyrim` | Logging, forms, TXT groups, animation, geometry, drawing, Papyrus, serialization | `clib-util`, `spdlog` |
| `hooks` | Prologue hooks | Detours |
| `json` | JSON fields | RapidJSON |
| `yaml-skyrim` | YAML form lists | `skyrim`, yaml-cpp |

With `default-features` false, omit `features` for the base package, including signing. The plain dependency `"clib-utils-qtr"` enables the Skyrim, hooks, JSON and YAML features. Include `CLibUtilsQTR/Signing.hpp` for signing (Windows x64 only); it needs no optional feature. The umbrella header `ClibUtilsQTR/utils.hpp` also includes dependency-heavy helpers.

For Skyrim code, keep your plugin's CommonLibVR-MIT and SKSE setup. The `skyrim` feature does not create a plugin target or initialize SKSE. Several engine headers expect engine declarations and standard headers from the plugin's PCH. Individual guides identify additional requirements.

## Find a helper

| Task | Guide | Names used in code |
| --- | --- | --- |
| Dynamic form creation and lifecycle | [Dynamic form tracking](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Dynamic-Form-Tracking) | `clib_utilsQTR` |
| Rotating logs | [Logging](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Logging) | `clib_utilsQTR` |
| Form resolution and TXT/YAML groups | [Forms and groups](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Forms-and-Groups) | `FormReader`, `PresetHelpers` |
| JSON, preset values, strings | [Configuration and strings](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Configuration-and-Strings) | `Presets`, `clib_utilsQTR`, `StringHelpers` |
| Background work | [Scheduling](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Scheduling) | `clib_utilsQTR::Tasker`, global `Ticker` |
| Mutex checks | [Debug locks](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Debug-Locks) | `clib_utilsQTR` |
| Function hooks | [Hooks](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Hooks) | `clib_utilsQTR` |
| Script calls | [Papyrus](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Papyrus) | `Papyrus` |
| Animation queues | [Animations](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Animations) | global `Animation`, `Animator` |
| Bounds and diagnostic shapes | [Geometry and drawing](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Geometry-and-Drawing) | `BoundingBox`, `DebugAPI_IMPL` |
| Serialization helpers | [Serialization](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Serialization) | `Serialization` |
| Verified DLL exports | [DLL Signing](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Signing) | `clib_utilsQTR::Signing` |

For authors contributing TXT group files, see the existing [Form Groups guide](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Form-Groups).
