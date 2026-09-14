#include <CLibUtilsQTR/PresetHelpers/YAMLTemplates.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <source_location>

void Require(bool value, const std::source_location where = std::source_location::current()) {
    if (!value) throw std::runtime_error("YAML template assertion failed at line " + std::to_string(where.line()));
}
YAML::Node Expand(const std::string& text) {
    auto node = YAML::Load(text);
    PresetHelpers::YAML_Helpers::ResolveTemplates(node);
    return node;
}
void Reject(const std::string& text, const std::string& message) {
    try { Expand(text); }
    catch (const YAML::Exception& error) { if (std::string(error.what()).find(message) == std::string::npos) throw std::runtime_error("Expected " + message + "; got " + error.what()); return; }
    throw std::runtime_error("Expected rejection: " + message);
}
bool Equal(const YAML::Node& a, const YAML::Node& b) {
    if (a.Type() != b.Type() || a.Tag() != b.Tag() || a.size() != b.size()) return false;
    if (a.IsScalar()) return a.Scalar() == b.Scalar();
    if (a.IsSequence()) {
        for (std::size_t i = 0; i < a.size(); ++i) if (!Equal(a[i], b[i])) return false;
    } else if (a.IsMap()) {
        for (const auto& entry : a) if (!Equal(entry.second, b[entry.first.Scalar()])) return false;
    }
    return true;
}
int main(int argc, char** argv) try {
    auto node = Expand(R"(
cooking: &cooking {duration: 0.06, sound: crack}
templates:
  food:
    parameters: [raw, cooked]
    body:
      forms: $raw
      transformers: [{<<: *cooking, finalFormEditorID: $cooked}]
rows: []
)");
    Require(!node["templates"]);
    try {
        auto bad = YAML::Load("templates: []");
        PresetHelpers::YAML_Helpers::ResolveTemplates(bad, "example.yml");
        Require(false);
    } catch (const YAML::Exception& error) {
        Require(std::string(error.what()).find("example.yml") != std::string::npos);
    }
    node = Expand(R"(
shared: &shared {duration: 6, nested: {value: original}}
templates:
  value:
    parameters: [input]
    body: $input
  food:
    parameters: [raw, cooked]
    body:
      forms: $raw
      transformers: [{<<: *shared, finalFormEditorID: $cooked, duration: 0}]
      literal: $$raw
  forward:
    parameters: [a, b]
    body: {use: food, args: [$a, $b]}
rows:
- {use: forward, args: [00065C99, "00000015"]}
- {use: food, args: [other, result]}
values:
- {use: value, args: [null]}
- {use: value, args: [0]}
- {use: value, args: [""]}
- {use: value, args: [false]}
- {use: value, args: [[a, b]]}
- {use: value, args: [{key: value}]}
- {use: value, args: ["$literal"]}
)");
    Require(node["rows"][0]["forms"].Scalar() == "00065C99");
    Require(node["rows"][0]["transformers"][0]["finalFormEditorID"].Scalar() == "00000015");
    Require(node["rows"][0]["transformers"][0]["finalFormEditorID"].Tag() == "!");
    Require(node["rows"][0]["literal"].Scalar() == "$raw");
    Require(node["rows"][0]["transformers"][0]["duration"].as<int>() == 0);
    node["rows"][0]["transformers"][0]["nested"]["value"] = "changed";
    Require(node["rows"][1]["transformers"][0]["nested"]["value"].Scalar() == "original");
    Require(node["shared"]["nested"]["value"].Scalar() == "original");
    Require(node["values"][0].IsNull());
    Require(node["values"][1].as<int>() == 0);
    Require(node["values"][2].Scalar().empty());
    Require(!node["values"][3].as<bool>());
    Require(node["values"][4].size() == 2 && node["values"][4].IsSequence());
    Require(node["values"][5]["key"].Scalar() == "value");
    Require(node["values"][6].Scalar() == "$literal");
    Require(Expand("use: ordinary\nargs: unchanged")["use"].Scalar() == "ordinary");
    Require(Expand("x: &x {v: 0}\ny: {<<: *x}")["y"]["v"].as<int>() == 0);
    Require(Expand("templates: {v: {parameters: [], body: null}}\nx: {use: v, args: []}")["x"].IsNull());
    Reject("templates: []", "must be a mapping");
    Reject("templates: {v: {body: 1}}", "requires");
    Reject("templates: {v: {parameters: [x, x], body: 1}}", "Duplicate parameter");
    Reject("templates: {v: {parameters: ['$x'], body: 1}}", "Invalid parameter");
    Reject("templates: {v: {parameters: [], body: 1, typo: 2}}", "Expected only");
    Reject("templates: {v: {parameters: [], body: 1}, v: {parameters: [], body: 2}}", "Duplicate YAML template");
    Reject("templates: {}\nx: {use: missing, args: []}", "Unknown YAML template");
    Reject("templates: {}\nx: {use: [], args: []}", "must be a name");
    Reject("templates: {v: {parameters: [x], body: $x}}\nx: {use: v, args: []}", "expects 1");
    Reject("templates: {v: {parameters: [], body: 1}}\nx: {use: v}", "expects 0");
    Reject("templates: {v: {parameters: [], body: 1}}\nx: {use: v, args: [], extra: 0}", "Expected only");
    Reject("templates: {v: {parameters: [], body: $missing}}\nx: {use: v, args: []}", "Unknown parameter");
    Reject("templates: {v: {parameters: [], body: {use: v, args: []}}}\nx: {use: v, args: []}", "Recursive");
    Reject("templates: {a: {parameters: [], body: {use: b, args: []}}, b: {parameters: [], body: {use: a, args: []}}}\nx: {use: a, args: []}", "Recursive");
    Reject("templates: {}\nx: &x [*x]", "Circular YAML alias");
    Reject("templates: {v: {parameters: [], parameters: [], body: 1}}", "Duplicate field");
    Reject("templates: {v: {parameters: [], body: 1, body: 2}}", "Duplicate field");
    Reject("templates: {v: {parameters: [], body: 1}}\nx: {use: v, use: other, args: []}", "Duplicate field");
    Reject("templates: {v: {parameters: [], body: 1}}\nx: {use: v, args: [], args: [2]}", "Duplicate field");
    Reject("templates: {ignore: {parameters: [x], body: 1}}\nx: {use: ignore, args: [{use: missing, args: []}]}", "Unknown YAML template");
    Reject("templates: {ignore: {parameters: [x], body: 1}, loop: {parameters: [], body: {use: loop, args: []}}}\nx: {use: ignore, args: [{use: loop, args: []}]}", "Recursive");
    Require(Expand("templates: {id: {parameters: [x], body: $x}}\nx: {use: id, args: [{use: id, args: [7]}]}")["x"].as<int>() == 7);
    auto aliases = Expand(R"(
templates:
  linked:
    parameters: [input]
    body: {base: &linked {v: $input}, alias: *linked}
rows: [{use: linked, args: [1]}, {use: linked, args: [2]}]
base: &outer {v: 3}
alias: *outer
)");
    Require(aliases["rows"][0]["base"].is(aliases["rows"][0]["alias"]));
    aliases["rows"][0]["base"]["v"] = 9;
    Require(aliases["rows"][0]["alias"]["v"].as<int>() == 9);
    Require(aliases["rows"][1]["alias"]["v"].as<int>() == 2);
    Require(aliases["base"].is(aliases["alias"]));
    auto mergedArgument = Expand(R"(
base: &defaults {value: 4, other: 7, empty: nonempty}
templates:
  defaults:
    parameters: []
    body: *defaults
  setting:
    parameters: [defaults]
    body: {<<: $defaults, value: 0, empty: null}
rows:
- {use: setting, args: [*defaults]}
- {use: setting, args: [{use: defaults, args: []}]}
)");
    for (const auto& row : mergedArgument["rows"]) {
        Require(row["value"].as<int>() == 0);
        Require(row["empty"].IsNull());
        Require(row["other"].as<int>() == 7);
        Require(!row["<<"]);
    }
    Reject("templates: {v: {parameters: [x], body: {<<: $x}}}\nx: {use: v, args: [4]}", "YAML merge requires");
    Reject("templates: {v: {parameters: [], body: &cycle [*cycle]}}\nx: {use: v, args: []}", "Circular YAML alias");
    if (argc == 3) {
        std::size_t count = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[1])) {
            if (entry.path().extension() != ".yml") continue;
            auto before = YAML::LoadFile(entry.path().string());
            auto after = YAML::LoadFile((std::filesystem::path(argv[2]) / std::filesystem::relative(entry.path(), argv[1])).string());
            PresetHelpers::YAML_Helpers::ResolveTemplates(before);
            PresetHelpers::YAML_Helpers::ResolveTemplates(after);
            Require(Equal(before["formsLists"], after["formsLists"]));
            ++count;
        }
        std::cout << count << " generated presets have identical expanded rules\n";
    }
    std::cout << "YAML template tests passed\n";
}

catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
