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
