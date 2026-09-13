# CLibUtilsQTR

A header-only C++23 utility library for Skyrim plugins and general C++ projects.

## Installation (via vcpkg)

You need CMake, vcpkg, and a C++23 compiler on Windows. Modules that depend on Skyrim require CommonLib.

1. **Copy the package files into your project.**

   | Source directory | Destination |
   | --- | --- |
   | [cmake/ports/clib-utils-qtr](cmake/ports/clib-utils-qtr) from this repository | `cmake/ports/clib-utils-qtr` |
   | [PO3's clib-util port](https://github.com/powerof3/CLibUtil/tree/master/cmake/ports/clib-util), if not already in your project | `cmake/ports/clib-util` |

   Each directory contains `portfile.cmake` and `vcpkg.json`. Copy both unchanged; they define how vcpkg downloads and installs the package.

2. **Register the copied ports.** Add this to your project's `vcpkg-configuration.json`:

   ```json
   { "overlay-ports": ["cmake/ports"] }
   ```

   Skip this if your CMake preset already points `VCPKG_OVERLAY_PORTS` at that folder.

3. **Add QTR to your project's `vcpkg.json`:**

   ```json
   {
     "dependencies": ["clib-utils-qtr"]
   }
   ```

   No feature list is needed to get everything. Merge these JSON entries into existing files, keeping your other settings.

   If your project has no vcpkg baseline, run `vcpkg x-update-baseline --add-initial-baseline` once from the project directory. This records the package versions to use. Keep an existing baseline unchanged.

4. **Add QTR's headers to your CMake target.** Add the following after your `add_library(...)` or `add_executable(...)` call. Replace `your_target` with that target's name and keep any equivalent settings you already have.

   ```cmake
   find_path(CLIB_UTILS_QTR_INCLUDE_DIRS "CLibUtilsQTR/utils.hpp" REQUIRED)
   target_include_directories(your_target PRIVATE ${CLIB_UTILS_QTR_INCLUDE_DIRS})
   target_compile_features(your_target PRIVATE cxx_std_23)
   ```

   `find_path` locates the include directory for the whole library.

   **Optional links:** uncomment only the pairs for helpers you use. Leave a pair commented out if your project already provides that library.

   ```cmake
   # Only for Hooks.hpp (Detours hooks).
   # find_library(DETOURS_LIBRARY detours REQUIRED)
   # target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})

   # Only for logging; skip if CommonLib already provides spdlog.
   # find_package(spdlog CONFIG REQUIRED)
   # target_link_libraries(your_target PRIVATE spdlog::spdlog)

   # Only for YAML helpers.
   # find_package(yaml-cpp CONFIG REQUIRED)
   # target_link_libraries(your_target PRIVATE yaml-cpp::yaml-cpp)
   ```

5. **Configure and build.** Use your existing vcpkg-enabled CMake preset. If you do not have one, run these commands from the project directory, replacing `<vcpkg>` with your vcpkg installation path:

   ```sh
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg>/scripts/buildsystems/vcpkg.cmake"
   cmake --build build
   ```

   Run them in a terminal where your compiler and build tools are available, such as a Visual Studio developer terminal. CMake installs the packages during configuration.

QTR is now available to your target. Include the headers for the helpers you use; Skyrim headers belong after your plugin's engine PCH.

## Optional dependencies

The plain `"clib-utils-qtr"` dependency installs all default dependencies. Commenting out a CMake link does not change what vcpkg installs.

To omit QTR's YAML and Detours dependencies, for example, replace the QTR entry in your project's `vcpkg.json` with:

```json
{
  "name": "clib-utils-qtr",
  "default-features": false,
  "features": ["skyrim"]
}
```

See the wiki for the [available features](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#choose-dependencies) or the [shorter local port without module choices](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#a-local-port-that-always-installs-everything).

## Updating

Replace the two QTR port files together with those from the new release, then reconfigure CMake. If you customized the dependency list, apply that choice to the updated port.

## Guides

[Browse all helpers and usage examples](https://github.com/QTR-Modding/CLibUtilsQTR/wiki#guides).
