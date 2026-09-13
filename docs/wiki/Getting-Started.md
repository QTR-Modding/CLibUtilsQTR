# Getting started

For a first installation, follow the [README's four setup steps](https://github.com/QTR-Modding/CLibUtilsQTR#installation-via-vcpkg). They install the whole library and its default dependencies. No module selection is required.

This page covers optional dependency choices, linking, and updating. The helper guides below cover the code on `main`.

## Install with vcpkg

The [installation steps](https://github.com/QTR-Modding/CLibUtilsQTR#installation-via-vcpkg) use a **local port**: two files copied into your project that tell vcpkg how to download and install QTR.

Two files share the name `vcpkg.json`, but have different jobs:

| File in your project | Purpose |
| --- | --- |
| `vcpkg.json` | Requests the packages your project uses. `"clib-utils-qtr"` requests QTR with all default dependencies. |
| `cmake/ports/clib-utils-qtr/vcpkg.json` | Defines QTR's dependencies and optional modules. Copy it with `portfile.cmake`; you do not need to write the feature definitions yourself. |

Keep the supplied port unchanged if you want optional module selection. For a shorter port that always installs everything, see [the alternative below](#a-local-port-that-always-installs-everything).

## Choose dependencies

A vcpkg **feature** is a named group of dependencies. All QTR headers are installed whichever features you select; features do not enable runtime behavior or remove C++ functions.

Leave the plain `"clib-utils-qtr"` dependency alone to get everything. To use only the Skyrim helpers, replace that entry in your project's dependency list with:

```json
{
  "name": "clib-utils-qtr",
  "default-features": false,
  "features": ["skyrim"]
}
```

`default-features: false` turns off the default selection; `features` chooses what to install instead. Add other names to that array if needed. Without `default-features: false`, your selection adds to the defaults.

The supplied local port offers:

| Feature | Helpers | Dependencies installed |
| --- | --- | --- |
| Base (always included) | Strings, debug locks, Tasker, Ticker, preset values, DLL signing | None |
| `skyrim` | Logging, forms including dynamic form tracking, TXT groups, animation, geometry, drawing, Papyrus, serialization | `clib-util`, `spdlog` |
| `hooks` | Prologue hooks | `detours` |
| `json` | JSON fields | `rapidjson` |
| `yaml-skyrim` | YAML form lists and merge keys | `skyrim`, `yaml-cpp` |

For only the base helpers, use:

```json
{ "name": "clib-utils-qtr", "default-features": false }
```

Include the specific headers you use. `CLibUtilsQTR/utils.hpp` includes helpers that need the optional dependencies and Skyrim declarations. Signing has its own `CLibUtilsQTR/Signing.hpp` header; it needs Windows x64 and the Windows SDK, but no external package.

Skyrim helpers assume your plugin already uses CommonLibVR-MIT and SKSE. Include them after your plugin's engine PCH. Installing the `skyrim` dependencies does not create a plugin target or initialize SKSE.

## Link the libraries you use

Installing a dependency makes it available to CMake; it does not automatically link it to your target. Keep any links your project already has. Add the following only for helpers you use, replacing `your_target` with your target's name.

For prologue hooks:

```cmake
find_library(DETOURS_LIBRARY detours REQUIRED)
target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})
```

For YAML presets or merge keys:

```cmake
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE yaml-cpp::yaml-cpp)
```

For logging, if spdlog is not already linked through your plugin setup:

```cmake
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE spdlog::spdlog)
```

## A local port that always installs everything

Use this alternative only if you want to maintain a simpler local port with no module selection. In **`cmake/ports/clib-utils-qtr/vcpkg.json`**, remove `features` and `default-features` and add this top-level field:

```json
"dependencies": [
  "clib-util",
  "detours",
  "rapidjson",
  "spdlog",
  "yaml-cpp"
]
```

Keep the file's other metadata and `portfile.cmake` unchanged. The port still installs all headers, and now always installs all five dependencies. Consumers of this port cannot reduce the dependency set with feature selection.

This list covers the current library. When updating your custom port, check whether the new version adds dependencies. Copying the supplied port unchanged avoids maintaining this list yourself.

## Update QTR

Replace both local port files together with the versions from the QTR release you want to use, then reconfigure CMake. `portfile.cmake` pins the downloaded source and its checksum; changing only the version text in `vcpkg.json` does not update the headers.

If you customized the port, apply your changes to the updated package definition. Preserve an existing PO3 `clib-util` pin unless you intend to update that dependency too.

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
