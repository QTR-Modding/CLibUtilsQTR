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
    auto undefinedDuplicate = YAML::Load("{<<: {x: 3}, x: 1, x: 2}");
    undefinedDuplicate["x"] = YAML::Node(YAML::NodeType::Undefined);
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(undefinedDuplicate);
    Require(undefinedDuplicate["x"].as<int>() == 3);
    auto nullKey = YAML::Load("{<<: {'null': merged, '': empty}, null: explicit}");
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(nullKey);
    Require(nullKey["null"].Scalar() == "merged" && nullKey[""].Scalar() == "empty");
    YAML::Node programmatic(YAML::NodeType::Sequence);
    for (int value = 0; value < 200; ++value) {
        YAML::Node row(YAML::NodeType::Map);
        YAML::Node defaults(YAML::NodeType::Map);
        defaults["value"] = value;
        YAML::Node mergeKey("<<");
        mergeKey.SetTag("tag:yaml.org,2002:merge");
        row[mergeKey] = defaults;
        programmatic.push_back(row);
    }
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(programmatic);
    for (int value = 0; value < 200; ++value) Require(programmatic[value]["value"].as<int>() == value);
    YAML::Node separate(YAML::NodeType::Sequence);
    separate.push_back(YAML::Load("{<<: {x: 1}}"));
    separate.push_back(YAML::Load("{<<: {x: 2}}"));
    separate.push_back(YAML::Clone(YAML::Load("{<<: {x: 3}}")));
    auto shared = YAML::Load("{<<: {x: 4}}");
    separate.push_back(shared);
    separate.push_back(shared);
    PresetHelpers::YAML_Helpers::ResolveMergeKeys(separate);
    for (int value = 0; value < 4; ++value) Require(separate[value]["x"].as<int>() == value + 1);
    Require(separate[3].is(separate[4]));
    YAML::Node cycle(YAML::NodeType::Sequence);
    cycle.push_back(cycle);
    bool rejectedCycle = false;
    try { PresetHelpers::YAML_Helpers::ResolveMergeKeys(cycle); }
    catch (const YAML::Exception&) { rejectedCycle = true; }
    Require(rejectedCycle);
    std::cout << "YAML merge tests passed\n";
}
