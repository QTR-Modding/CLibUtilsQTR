# CLibUtilsQTR

C++23 helpers for Skyrim plugins and general C++ code, including forms, logging, configuration, hooks, and background tasks. The library is header-only: include the helpers you need; there is no QTR DLL to build or distribute.

## Installation (via vcpkg)

These steps add the whole library to an existing CMake project. You need vcpkg and a C++23 compiler. Skyrim helpers also need your project's CommonLibVR-MIT/SKSE setup.

1. Copy [cmake/ports/clib-utils-qtr](cmake/ports/clib-utils-qtr) into the same path in your project. Keep both `portfile.cmake` and `vcpkg.json` together. They tell vcpkg where to download QTR and which dependencies to install; you can use them unchanged.

   If your project does not already supply `clib-util`, also copy [PO3's clib-util port](https://github.com/powerof3/CLibUtil/tree/master/cmake/ports/clib-util) into `cmake/ports/clib-util`.

2. Register that folder in your project's `vcpkg-configuration.json`:

   ```json
   { "overlay-ports": ["cmake/ports"] }
   ```

   This makes vcpkg find the copied packages. If your CMake preset already sets `VCPKG_OVERLAY_PORTS` to this folder, skip this step.

3. Add QTR to your project's `vcpkg.json`:

   ```json
   {
     "dependencies": ["clib-utils-qtr"]
   }
   ```

   This installs **all QTR headers and all default dependencies**. No feature list is needed. Merge these entries into existing JSON files rather than replacing the files.

   If your project has no package baseline yet, run `vcpkg x-update-baseline --add-initial-baseline` from its directory. This records the versions of vcpkg packages to use; keep an existing baseline unchanged.

4. After creating your target in `CMakeLists.txt`, add:

   ```cmake
   find_path(CLIB_UTILS_QTR_INCLUDE_DIRS "CLibUtilsQTR/StringHelpers.hpp" REQUIRED)
   target_include_directories(your_target PRIVATE ${CLIB_UTILS_QTR_INCLUDE_DIRS})
   target_compile_features(your_target PRIVATE cxx_std_23)
   ```

   Replace `your_target` with your executable or plugin target's name. Configure CMake using your existing vcpkg preset. If vcpkg is not wired into CMake yet, follow [vcpkg's CMake setup](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration).

You can now include a helper:

```cpp
#include <CLibUtilsQTR/StringHelpers.hpp>

const auto name = StringHelpers::trim("  My preset  ");
// name == "My preset"
```

Some helpers also require linking their external library. See the wiki's [linking examples](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#link-the-libraries-you-use) when using hooks, logging, or YAML.

## Optional dependencies

The setup above gets everything. To install fewer dependencies, see [Choose dependencies](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#choose-dependencies).

If you maintain your own local port and want a short package definition without module choices, see [A local port that always installs everything](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#a-local-port-that-always-installs-everything).

## Logging

[Logging guide](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Logging): call `clib_utilsQTR::SetupLog()` for rotating logs, with optional settings.

## Debug lock guards

[Debug locks guide](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Debug-Locks): detect recursive locks, invalid unlocks, and lock-order violations.

For all other helpers, see the [guide index](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#find-a-helper).
