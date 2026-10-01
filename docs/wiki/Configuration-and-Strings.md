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

Define a reusable value under `templates`, then call it with the values you want to insert:

```yaml
templates:
  soundSettings:
    duration: $duration
    sound: $soundName

settings:
  <<: soundSettings(6, Crack)
  duration: 2
```

`soundSettings` is the template's name. Its contents are the value it produces. `$duration` and `$soundName` are parameters: placeholders filled by the arguments inside the parentheses.

Arguments follow the **first appearance of each distinct parameter**, reading values from top to bottom, including nested values. Here, `6` fills `$duration` and `Crack` fills `$soundName`. Repeated occurrences of a parameter use the same argument. Reordering the first occurrences changes the argument order.

The call produces `{duration: 6, sound: Crack}`. The `<<` merges those fields into `settings`, where the explicit duration overrides `6`:

```yaml
settings:
  duration: 2
  sound: Crack
```

### Use a call directly

A call can replace an entire value. For repeated entries, put one call on each line:

```yaml
templates:
  setting:
    name: $name
    value: $value

settings:
  - setting(distance, 100)
  - setting(enabled, false)
```

This produces:

```yaml
settings:
  - name: distance
    value: 100
  - name: enabled
    value: false
```

A template with no parameters is called with empty parentheses, such as `defaults()`. Supply exactly one argument for each distinct parameter.

### Strings, lists, and mappings

Arguments use YAML value syntax. Numbers, booleans, nulls, lists, and mappings retain their types. Quote an argument to keep it a string or to include commas:

```yaml
templates:
  value: $input

values:
  - value(0)
  - value(false)
  - value(null)
  - value("")
  - value("123")
  - value("Crack, then (hiss)")
  - value([first, second])
  - 'value({duration: 2, sound: Crack})'
```

The last call has quotes around the **whole call** because `: ` has a special meaning in the outer YAML document. Also quote whole calls when writing them inside YAML flow lists or mappings: `["setting(distance, 100)", "setting(enabled, false)"]`.

A parameter replaces a whole value. `sound: $soundName` substitutes the argument; `sound: "Playing $soundName"` keeps that text as written. Field names are literal. Use `$$soundName` in a template to produce the literal `$soundName`.

Template and parameter names use letters, digits, underscores, and hyphens. Write calls as `name(...)`, with the opening parenthesis immediately after the name.

### Merges, anchors, and nested calls

Calls are expanded before their results are merged. The usual [YAML merge rules](#yaml-templates-with-merge-keys) apply: explicit fields win, and earlier sources win in `<<: ["first()", "second()"]`. A call used as a merge source must produce a mapping, or a list of mappings.

A mapping argument can supply merge fields inside a template:

```yaml
templates:
  setting:
    <<: $defaults
    duration: $duration

settings: 'setting({duration: 6, sound: Crack}, 2)'
```

The result is `{duration: 2, sound: Crack}`.

Template definitions can contain YAML anchors and aliases. Each call copies the definition, preserving links between its aliases.

Arguments are parsed separately from the document, so they cannot use its anchors. Pass a mapping directly or put the anchor inside a template.

To call another template from a definition:

```yaml
templates:
  soundSettings:
    duration: $duration
    sound: $soundName
  crackSettings: soundSettings($duration, Crack)

settings: crackSettings(2)
```

Parameters inside the nested call's arguments also count toward the enclosing template's parameter order. Here, `crackSettings` has one parameter: `$duration`.

A call can supply another call's argument: `value("crackSettings(2)")`. Here, `crackSettings(2)` runs first, and `value` receives its result. Quote the inner call to keep any commas inside one argument.

A definition cannot call itself, directly or through another template. Calls nested in arguments are allowed, such as `value("value(2)")`.

A template may produce a scalar, mapping, or list. A list result stays one value: placing it inside another list produces a nested list.

### Call recognition and definitions

Calls must occupy a whole YAML value and start with a defined template name followed by `(`. Quoting a call still allows expansion. To keep the text unchanged, write `label: !!str soundSettings(6, Crack)`. Unknown template names remain ordinary text.

Keep definitions in one top-level `templates` mapping; they are available only within that document. Duplicate sections or template names are errors.

A root-level YAML merge can supply `templates`. An explicit `templates` mapping replaces the inherited one. When merging several sources, the first takes precedence.

After expansion, the helper removes the top-level definitions. Documents without `templates` continue through ordinary merge resolution.

### Enable templates in C++

Applications enable this QTR syntax after loading YAML and before reading configuration fields. Use C++23 and yaml-cpp:

```cpp
#include <CLibUtilsQTR/PresetHelpers/YAMLTemplates.hpp>

auto config = YAML::LoadFile("preset.yml");
PresetHelpers::YAML_Helpers::ResolveTemplates(config, "preset.yml");
// Read the expanded config here.
```

The second argument is optional and adds the filename to errors. This call also resolves merge keys, so it replaces an existing `ResolveMergeKeys` call. Expansion happens once during configuration loading.

Catch `YAML::Exception` in the application's configuration error handler and discard the document if expansion fails. Invalid names, argument syntax, argument counts, merge values, recursive calls, and circular aliases produce errors. Parameter discovery inspects every definition when loading; calls and merges in a body are evaluated when that template is used.
