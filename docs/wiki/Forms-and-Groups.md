# Forms and groups

Resolve a configured form after Skyrim's form data is available:

```cpp
#include <CLibUtilsQTR/FormReader.hpp>

// With your plugin's engine PCH in scope:
RE::TESForm* form = FormReader::GetFormFromString("FoodBeef");
if (form) {
    const RE::FormID id = form->GetFormID();
    // Use the resolved form.
}
```

A form is an engine record, such as an ingredient or location. A placed object or actor is a reference. Keep an engine handle when a transient reference must survive beyond the current call. These helpers use the `skyrim` feature.

## Accepted identifiers

| Input | Meaning |
| --- | --- |
| `FoodBeef` | Editor ID |
| `000669A3` | Full hexadecimal FormID |
| `0x000669A3` | Full hexadecimal FormID with prefix |
| `000669A3~Skyrim.esm` | Local hexadecimal ID and defining plugin |

`GetFormFromString()` returns a pointer or `nullptr`. `GetFormEditorIDFromString()` returns an ID or `0`. `GetFormByID(id, editor_id)` tries a nonempty editor ID first, then the numeric ID. `GetEditorID(id)` returns an editor ID when available. None of these functions waits for records to load.

## TXT groups

A group is a named set of FormIDs. Each `.txt` file supplies one group, named after its filename without the extension:

```cpp
#include <CLibUtilsQTR/PresetHelpers/PresetHelpersTXT.hpp>

// Once form data is available:
PresetHelpers::TXT_Helpers::GatherForms(
    "Data/SKSE/Plugins/MyPlugin/FormGroups");
```

Example `Foods.txt`:

```text
FoodBeef
000669A3~Skyrim.esm
```

The loader trims lines, skips blank or unresolved entries, and removes duplicates within a group. It scans the immediate folder, not subdirectories. TXT files do not support comments or nested group references.

Each scanned file replaces its named group. Reloading does not remove groups whose files have disappeared. Load groups during configuration initialization before runtime consumers use them. The existing [Form Groups guide](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Form-Groups) explains the author-facing format.

## YAML expansion

After loading TXT groups:

```cpp
#include <CLibUtilsQTR/PresetHelpers/PresetHelpersYAML.hpp>

const auto config = YAML::Load(R"(
allowed:
  - Foods
  - FoodBeef
)");
const auto forms =
    PresetHelpers::YAML_Helpers::CollectFrom<RE::FormID, std::string>(
        config, "allowed");
```

Use `yaml-skyrim` and link yaml-cpp:

```cmake
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE yaml-cpp::yaml-cpp)
```

Each string is checked as a group name first, then as a single form identifier. Unresolved strings contribute no IDs. Group order is unspecified; overlapping groups and repeated tokens can produce duplicates in the returned vector.

`StringToFormIDs(text)` expands one string. For ordinary YAML values, `CollectFrom<T>(node, key)` accepts a scalar or sequence. Handle YAML parsing and conversion exceptions in your caller.
