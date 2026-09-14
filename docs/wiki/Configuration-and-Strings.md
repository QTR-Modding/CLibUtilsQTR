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

These helpers need yaml-cpp, included by the supplied port's default `yaml-skyrim` feature. [Link yaml-cpp](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started#link-the-libraries-you-use), then resolve merges once after parsing, before reading fields:

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

## Parameterized YAML templates

A template is a reusable YAML value with named inputs. Each `use` call replaces itself with a copy of the template's `body`:

```yaml
templates:
  setting:
    parameters: [name, value]
    body:
      name: $name
      value: $value

settings:
- use: setting
  args: [distance, 100]
- use: setting
  args: [enabled, false]
```

The resulting `settings` contains `{name: distance, value: 100}` and `{name: enabled, value: false}`. This is a QTR extension, not standard YAML syntax. Applications opt in with:

```cpp
#include <CLibUtilsQTR/PresetHelpers/YAMLTemplates.hpp>

auto config = YAML::LoadFile("preset.yml");
PresetHelpers::YAML_Helpers::ResolveTemplates(config, "preset.yml");
// Read config normally here.
```

The optional second argument adds a filename or other source label to errors. This public header needs only yaml-cpp and C++23; it has no Skyrim dependency. It resolves merge keys too, so replace an existing `ResolveMergeKeys()` call rather than calling both helpers.

1. Define templates in the document's top-level `templates` mapping. Each definition has exactly `parameters` and `body`. Parameter names are nonempty, unique strings without a leading `$`.
2. Calls have exactly `use` and `args`. Arguments are a sequence in parameter order; use `args: []` for a template with no parameters.
3. `$name` replaces a whole scalar value in the body. It does not interpolate part of a string or replace mapping keys. Arguments retain their YAML types, including null, zero, false, empty strings, sequences and mappings. Use `$$name` in a body to produce the literal string `$name`.
4. Bodies can reuse anchors and merge keys, and call other templates. Arguments are copied, so calls produce independent results. A sequence body becomes a sequence value; it is not flattened into its surrounding list.
5. Templates belong to one document. Expansion removes the top-level definitions. Without a `templates` key, ordinary `use` and `args` fields are untouched. With templates enabled, mappings containing `use` are reserved for calls throughout the document, including supplied arguments.
6. Invalid definitions, unknown templates or parameters, wrong argument counts, circular aliases and recursive calls throw `YAML::Exception`. Template bodies are expanded when called; errors in unused bodies are not evaluated. Discard the document on failure and report the exception through your application's config error handling.

Expansion runs during configuration loading. Applications consume ordinary nodes afterward; no template machinery is needed at runtime.
