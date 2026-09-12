# Configuration and strings

## JSON fields with defaults

A `Field` associates a JSON property name with a C++ value:

```cpp
#include <rapidjson/document.h>
#include <CLibUtilsQTR/PresetHelpers/Config.hpp>

rapidjson::Document config;
config.Parse(R"({"enabled": true})");
Presets::Field<bool, rapidjson::Value> enabled{"enabled", false};
if (!config.HasParseError() && config.IsObject()) {
    enabled.load(config);
}
const bool useFeature = enabled.get();
```

Install `json`. Use `rapidjson::Value` as the block type even when loading from a `Document`. `load()` returns success; missing or incompatible values preserve the previous value. Supply an explicit default, especially for scalars. The helper reads a parsed object, not a file.

JSON getters support strings, booleans, supported integer and floating-point types, and vectors of those scalar types. Numbers must pass RapidJSON's type checks; strings are not converted to numbers.

For direct reads use `Presets::Getters::JSON::Get(value, output)` or `Get(object, key, output)`. Direct vector reads append and can leave partial additions on failure. `Field::load()` uses a temporary before replacing its stored value.

## Fixed preset values

A `PresetPool` names a fixed set of choices. A `PresetSetting` stores a value for each:

```cpp
#include <CLibUtilsQTR/PresetSettings.hpp>

inline constexpr clib_utilsQTR::PresetPool<3> quality{
    std::array<std::string_view, 3>{"Low", "Medium", "High"}
};
clib_utilsQTR::PresetSetting<float, 3, quality> distance{
    std::array<float, 3>{100.0f, 200.0f, 400.0f}
};
distance.set_level(1);
const float selected = distance.current;   // 200.0f
const float high = distance.for_level(2);   // 400.0f
```

These types need only the base package. Indices are zero-based and must stay within the declared size. `set_level()` changes the current value; `for_level()` only reads a preset value. Changing a pool's `current` field does not automatically update its settings.

## Strings

```cpp
#include <CLibUtilsQTR/StringHelpers.hpp>

const auto clean = StringHelpers::trim("  Foods  ");
const auto lower = StringHelpers::toLowercase(clean);
const auto joined = StringHelpers::join(
    std::vector<std::string>{"Beef", "Bread"}, ", ");
```

| Function | Behavior |
| --- | --- |
| `trim(text)` | Removes leading/trailing spaces, tabs, carriage returns, newlines |
| `toLowercase(text)` | Lowercases bytes with `std::tolower` |
| `join(values, delimiter)` | Streams values into a delimited string |
| `replaceLineBreaksWithSpace(text)` | Replaces newline characters with spaces |
| `includesString(text, candidates)` | Tests for any case-insensitive substring |
| `includesWord(text, candidates)` | Tests case-insensitive, space-delimited words or phrases |

`includesWord()` is not general punctuation-aware or Unicode word segmentation.

## YAML templates with merge keys

Install the `yaml` feature (`yaml-skyrim` includes it). Resolve merges once after parsing, before reading fields:

```cpp
#include <CLibUtilsQTR/PresetHelpers/YAMLMerge.hpp>

auto config = YAML::LoadFile("preset.yml");
PresetHelpers::YAML_Helpers::ResolveMergeKeys(config);
```

A merge key (`<<`) copies shared fields into a mapping:

```yaml
fire: &fire
  duration: 0.06
  sound: ThawSound

transformers:
  - <<: *fire
    finalFormEditorID: FoodBeef
  - <<: *fire
    finalFormEditorID: FoodMammothMeat
    duration: 0.1
```

Explicit fields win, including null and zero. `<<: [*first, *second]` accepts multiple templates; earlier templates win when both define a field. Merges are shallow: an explicit nested mapping replaces the inherited mapping. Nested merge directives are resolved too. Quoted `"<<"` remains an ordinary key.

The helper updates the document in place and preserves aliases. It accepts scalar mapping keys. Invalid merge values and circular aliases throw `YAML::Exception`; discard the document if resolution fails. Catch this alongside your usual YAML parsing errors.
