# Getting started

These steps add QTR to an existing CMake project using vcpkg. They install all QTR headers and dependencies. Optional module selection comes afterward.

You need CMake, vcpkg, and a C++23 compiler. Modules that depend on Skyrim require CommonLib. The guides describe the code on `main`.

## Install with vcpkg

1. **Copy the package files.** A vcpkg *port* is a package definition: it tells vcpkg where to download a library and what it depends on. Copy these directories into your project:

   | Copy from | Destination in your project |
   | --- | --- |
   | [QTR's port](https://github.com/QTR-Modding/CLibUtilsQTR/tree/main/cmake/ports/clib-utils-qtr) | `cmake/ports/clib-utils-qtr` |
   | [PO3's CLibUtil port](https://github.com/powerof3/CLibUtil/tree/master/cmake/ports/clib-util), if your project does not already supply it | `cmake/ports/clib-util` |

   Each directory contains `portfile.cmake` and `vcpkg.json`. Copy both files unchanged. The `vcpkg.json` inside the port defines the package; your project's own `vcpkg.json` requests packages in step 3.

2. **Tell vcpkg where those ports are.** Add this to `vcpkg-configuration.json` in your project directory:

   ```json
   { "overlay-ports": ["cmake/ports"] }
   ```

   If your CMake preset already sets `VCPKG_OVERLAY_PORTS` to that directory, this is already configured. Keep one of these methods.

3. **Request QTR.** Add `"clib-utils-qtr"` to the dependencies in your project's `vcpkg.json`:

   ```json
   {
     "dependencies": ["clib-utils-qtr"]
   }
   ```

   That requests the whole library with all default dependencies. No `features` or `default-features` setting is needed. For both JSON files, merge the entries into existing configuration rather than replacing other settings.

   If the project has no vcpkg baseline yet, run this once from the project directory:

   ```sh
   vcpkg x-update-baseline --add-initial-baseline
   ```

   A baseline records the versions of vcpkg packages to use. Preserve an existing baseline.

4. **Make the headers available to your CMake target.** After your `add_library(...)` or `add_executable(...)` call, add:

   ```cmake
   find_path(CLIB_UTILS_QTR_INCLUDE_DIRS "CLibUtilsQTR/utils.hpp" REQUIRED)
   target_include_directories(your_target PRIVATE ${CLIB_UTILS_QTR_INCLUDE_DIRS})
   target_compile_features(your_target PRIVATE cxx_std_23)
   ```

   Replace `your_target` with the target's name. Keep equivalent settings your project already has. `utils.hpp` is only the file CMake searches for to locate QTR's include directory; this does not include that header or select any helpers.

Configure and build with your existing vcpkg-enabled CMake preset. If you have no such preset, configure with vcpkg's toolchain file instead, replacing `<vcpkg>` with your vcpkg installation directory:

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg>/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

CMake runs vcpkg to install the packages during configuration. For initial vcpkg setup, see [vcpkg's CMake instructions](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration).

## Use a helper

Choose the operation you need in the [guide index](https://github.com/QTR-Modding/CLibUtilsQTR/wiki#guides). Each guide shows the header to include and a usage example. Installing the whole library does not require including every header.

Skyrim headers expect your plugin's engine declarations to be available first, usually through its PCH. `CLibUtilsQTR/utils.hpp` collects many of those headers. Prefer the specific headers shown in the guides, particularly in programs that do not use Skyrim.

## Link the libraries you use

QTR itself is header-only. Some helpers call compiled dependency libraries, so using those helpers requires the corresponding CMake link below. Skip any link your project already provides; replace `your_target` with your target's name.

**Prologue hooks need Detours:**

```cmake
find_library(DETOURS_LIBRARY detours REQUIRED)
target_link_libraries(your_target PRIVATE ${DETOURS_LIBRARY})
```

**YAML helpers need yaml-cpp:**

```cmake
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE yaml-cpp::yaml-cpp)
```

**Logging needs spdlog**, which your CommonLib setup may already provide:

```cmake
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE spdlog::spdlog)
```

## Choose dependencies

This section is optional. A vcpkg *feature* selects a group of external dependencies. All headers are installed regardless of the selection. Feature names in the helper guides matter only when you want fewer dependencies than the default installation.

For example, to request only the dependencies for Skyrim helpers, replace the QTR entry in your project's `vcpkg.json` with:

```json
{
  "name": "clib-utils-qtr",
  "default-features": false,
  "features": ["skyrim"]
}
```

`default-features: false` turns off the default dependency groups. `features` selects what to install instead; add more names to that array as needed. Leave the copied port files unchanged.

The supplied port supports these groups:

| Feature | Helpers | Dependencies |
| --- | --- | --- |
| Base, always included | Strings, debug locks, Tasker, Ticker, preset values, DLL signing | No external packages |
| `skyrim` | Forms and dynamic forms, logging, animation, geometry, drawing, Papyrus, serialization, TXT groups | `clib-util`, `spdlog` |
| `hooks` | Prologue hooks | `detours` |
| `json` | JSON fields and getters | `rapidjson` |
| `yaml-skyrim` | YAML form lists and merge keys | `skyrim`, `yaml-cpp` |

For just the base helpers, use:

```json
{ "name": "clib-utils-qtr", "default-features": false }
```

Individual headers can have platform requirements even without package dependencies. For example, `Signing.hpp` requires Windows x64 and the Windows SDK.

## A local port that always installs everything

If you prefer a short package definition without optional modules, you can simplify **`cmake/ports/clib-utils-qtr/vcpkg.json`**. Remove its `features` and `default-features` fields and add this top-level field:

```json
"dependencies": [
  "clib-util",
  "detours",
  "rapidjson",
  "spdlog",
  "yaml-cpp"
]
```

Keep its other metadata and `portfile.cmake` unchanged. This port always installs all five dependencies; consumers of it cannot select a smaller set through QTR features. The supplied port's defaults install the same set, while retaining that choice.

When updating this customized port, check whether the new library version adds dependencies to the list.

## Update QTR

Copy the two port files from the new QTR release together, then reconfigure CMake. `portfile.cmake` contains the downloaded source revision and checksum; changing only the version in `vcpkg.json` does not update the headers.

If you customized the dependency list, apply that choice to the new package definition. Update PO3's CLibUtil port separately if needed; a QTR update does not require replacing an existing CLibUtil pin automatically.

## Find a helper

The [complete guide index](https://github.com/QTR-Modding/CLibUtilsQTR/wiki#guides) lists each area of the library with its usage guide.
