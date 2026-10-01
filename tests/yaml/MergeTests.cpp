#include <CLibUtilsQTR/PresetHelpers/YAMLMerge.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>

void Require(bool result) { if (!result) throw std::runtime_error("YAML merge assertion failed"); }
int main() {
    auto config = YAML::Load(R"(
first: &first {duration: 6, sound: crack, nested: {a: 1}}
second: &second {duration: 9, color: red}
rows:
  - <<: [*first, *second]
    duration: 0
    sound: null
    nested: {b: 2}
  - &derived
    <<: *first
    duration: .inf
  - <<: *derived
literal: {"<<": keep}
)");
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(config);
    auto rows = config["rows"];
    Require(rows[0]["duration"].as<int>() == 0);
    Require(rows[0]["sound"].IsNull());
    Require(rows[0]["color"].as<std::string>() == "red");
    Require(!rows[0]["nested"]["a"] && rows[0]["nested"]["b"].as<int>() == 2);
    Require(rows[2]["duration"].as<float>() == std::numeric_limits<float>::infinity());
    Require(rows[2]["sound"].as<std::string>() == "crack");
    Require(!rows[2]["<<"] && config["literal"]["<<"].as<std::string>() == "keep");
    Require(config["first"]["duration"].as<int>() == 6);
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(config);
    for (const auto* text : {"{<<: 1}", "{<<: [1]}", "&a {<<: *a}", "&a [*a]", "{<<: {}, <<: {}}"}) {
        bool failed = false;
        try { PresetHelpers::YAML_Helpers::ResolveMergeKeys(YAML::Load(text)); }
        catch (const YAML::Exception&) { failed = true; }
        Require(failed);
    }
    auto priority = YAML::Load("a: &a {x: 1}\nb: &b {x: 2}\nc: {<<: [*a, *b]}");
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(priority);
    Require(priority["c"]["x"].as<int>() == 1);
    auto undefined = YAML::Load("{<<: {value: 7}}");
    auto slot = undefined["value"];
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(undefined);
    Require(slot.as<int>() == 7 && undefined["value"].as<int>() == 7);
    auto duplicateKeys = YAML::Load("{<<: [{x: 1, x: 2}, {x: 3}], y: 0}");
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(duplicateKeys);
    Require(duplicateKeys["x"].as<int>() == 1);
    auto nullKey = YAML::Load("{<<: {'null': merged, '': empty}, null: explicit}");
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(nullKey);
    Require(nullKey["null"].Scalar() == "merged" && nullKey[""].Scalar() == "empty");
    std::cout << "YAML merge tests passed\n";
}
