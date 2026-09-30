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

Use a template when several entries share the same structure but need different values. Define the structure once, give its inputs names, and supply the values in each call.

### Define a template and use it

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

`setting` is the template's name. `parameters` names its inputs, and `body` is the value it produces. Inside the body, `$name` and `$value` mark where the supplied values go.

`use: setting` calls the template. `args` supplies values in the same order as `parameters`: the first call assigns `distance` to `name` and `100` to `value`.

After expansion, the application reads:

```yaml
settings:
  - name: distance
    value: 100
  - name: enabled
    value: false
```

The helper removes the top-level `templates` section and replaces each call with its expanded body. Each call produces a separate copy.

### Supply arguments

1. Each definition has exactly two fields: `parameters` and `body`. Each call has exactly two fields: `use` and `args`.
2. `args` must be a list with one value for each parameter. For a template with no inputs, write `parameters: []` in the definition and `args: []` in the call.
3. Template names and parameter names must be nonempty. Parameter names must be unique within their definition and are written without the `$` prefix.
4. An argument can be a string, number, boolean, null, list, or mapping (a group of key/value fields). Its type is preserved: passing `false`, `0`, or `null` keeps that value. Quote a number such as `"123"` when it should remain a string.

A parameter replaces a **whole value**. For example, `name: $name` works. `name: "Item $name"` remains the literal text `Item $name`; it does not insert the argument into that text. Parameter substitution applies to values, not field names.

To produce a literal value starting with `$`, double the first dollar sign in the body: `name: $$name` produces `name: $name`. Write `name: $$price USD` to produce the literal `$price USD`; a value starting with a single `$` is treated as a parameter reference.

### Combine arguments with YAML merges

A body can use anchors and merge keys. Here, the `defaults` argument supplies fields to merge into the result:

```yaml
templates:
  timedSetting:
    parameters: [defaults, duration]
    body:
      <<: $defaults
      duration: $duration

settings:
  - use: timedSetting
    args: [{duration: 6, sound: Crack}, 2]
```

The result is:

```yaml
settings:
  - duration: 2
    sound: Crack
```

Arguments are substituted before the body's merge keys are resolved. The explicit `duration` therefore overrides the duration inherited from `defaults`, using the [merge rules above](#yaml-templates-with-merge-keys).

Anchors and aliases within a body still refer to the same copied value. Separate calls have separate copies. A body can also be a list; its call is replaced by that list as a single value. If the call is already inside another list, the result is a nested list.

### Call another template

A body or an argument can contain another `use` call. For example, given the `setting` template above, add this definition beside it:

```yaml
  distanceSetting:
    parameters: [distance]
    body:
      use: setting
      args: [distance, $distance]
```

Calling `{use: distanceSetting, args: [100]}` produces `{name: distance, value: 100}`.

Calls inside arguments are expanded before the receiving template's body, including arguments the body does not use. A body cannot call itself, directly or through another template. A finite nested call supplied as an argument, such as an identity template receiving the result of another identity call, is allowed.

### Where definitions and calls belong

Keep definitions in one top-level `templates` mapping. Templates belong to that YAML document. A root-level YAML merge can supply this mapping too: an explicit `templates` field wins over an inherited one, and the first merge source wins over later sources. Duplicate explicit `templates` fields and duplicate template names are errors.

When `templates` is present, the helper treats mappings containing `use` as calls throughout the document, including arguments. Reserve that field name for template calls in these documents. When `templates` is absent, the helper resolves ordinary YAML merges and leaves `use` and `args` as ordinary fields.

### Enable templates in C++

Applications enable this QTR syntax by calling `ResolveTemplates` after loading YAML and before reading configuration fields. Use C++23 and yaml-cpp, and include the public header:

```cpp
#include <CLibUtilsQTR/PresetHelpers/YAMLTemplates.hpp>

auto config = YAML::LoadFile("preset.yml");
PresetHelpers::YAML_Helpers::ResolveTemplates(config, "preset.yml");
// Read the expanded config here.
```

The second argument is optional and adds the filename to error messages. `ResolveTemplates` also resolves YAML merge keys, so it replaces an existing `ResolveMergeKeys` call. Expansion happens once during configuration loading.

Catch `YAML::Exception` in the application's configuration error handler and discard the document if expansion fails. Errors include malformed definitions or calls, duplicate fields, unknown template or parameter names, incorrect argument counts, recursive calls, and circular YAML aliases. Definition structure is checked when loading the template bank; body contents are expanded and checked when that template is called.
