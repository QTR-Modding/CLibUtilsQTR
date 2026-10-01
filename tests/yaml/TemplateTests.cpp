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
    auto node = Expand(R"yaml(
templates:
  soundSettings:
    duration: $duration
    sound: $soundName
    repeated: $duration
settings:
  <<: soundSettings(6, Crack)
  duration: 2
)yaml");
    Require(node["settings"]["duration"].as<int>() == 2);
    Require(node["settings"]["sound"].Scalar() == "Crack");
    Require(node["settings"]["repeated"].as<int>() == 6);
    Require(!node["templates"]);
    node = Expand(R"yaml(
shared: &shared {duration: 6, nested: {value: original}}
templates:
  value: $input
  food:
    forms: $raw
    transformers: [{<<: *shared, finalFormEditorID: $cooked, duration: 0}]
    literal: $$raw
  forward: food($a, $b)
rows:
  - forward(00065C99, "00000015")
  - food(other, result)
values:
  - value(null)
  - value(0)
  - value("")
  - value(false)
  - value([a, b])
  - "value({key: value})"
  - value("$literal")
  - value("Crack, then (hiss)")
  - value('it''s crackling')
  - value("value(7)")
)yaml");
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
    Require(node["values"][7].Scalar() == "Crack, then (hiss)");
    Require(node["values"][8].Scalar() == "it's crackling");
    Require(node["values"][9].as<int>() == 7);
    auto aliases = Expand(R"yaml(
templates:
  linked: {base: &linked {v: $input}, alias: *linked}
rows: ["linked(1)", "linked(2)"]
base: &outer {v: 3}
alias: *outer
)yaml");
    Require(aliases["rows"][0]["base"].is(aliases["rows"][0]["alias"]));
    aliases["rows"][0]["base"]["v"] = 9;
    Require(aliases["rows"][0]["alias"]["v"].as<int>() == 9);
    Require(aliases["rows"][1]["alias"]["v"].as<int>() == 2);
    Require(aliases["base"].is(aliases["alias"]));
    auto merged = Expand(R"yaml(
templates:
  defaults: {value: 4, other: 7, empty: nonempty}
  setting: {<<: $defaults, value: 0, empty: null}
  nested:
    <<: defaults()
    value: 0
rows:
  - 'setting({value: 5, other: 7, empty: nonempty})'
  - setting("defaults()")
  - nested()
last: &last {other: 8, tail: yes}
row: {<<: ["defaults()", *last], value: 0}
<<: defaults()
)yaml");
    for (const auto& row : merged["rows"]) {
        Require(row["value"].as<int>() == 0);
        Require(row["other"].as<int>() == 7);
        Require(!row["<<"]);
    }
    Require(merged["rows"][0]["empty"].IsNull());
    Require(merged["row"]["other"].as<int>() == 7);
    Require(merged["row"]["tail"].Scalar() == "yes");
    Require(merged["other"].as<int>() == 7);
    auto inherited = Expand(R"yaml(
common: &common
  templates:
    setting: {<<: $defaults, value: 0}
<<: *common
row: "setting({value: 5, extra: 7})"
)yaml");
    Require(inherited["row"]["extra"].as<int>() == 7);
    Require(inherited["row"]["value"].as<int>() == 0);
    Require(Expand("templates: {v: null}\nx: v()")["x"].IsNull());
    Require(Expand("templates: {v: 1}\nx: !!str v()")["x"].Scalar() == "v()");
    Require(Expand("templates: {v: 1}\nx: unknown(2)")["x"].Scalar() == "unknown(2)");
    Require(Expand("templates: {v: 1}\nx: {use: ordinary, args: unchanged}")["x"]["use"].Scalar() == "ordinary");
    for (const auto& text : {"x: v(1)", "x: &x {v: 0}\ny: {<<: *x}", "x: {<<: [{v: 2}, {v: 3}], v: null}"}) {
        auto old = YAML::Load(text);
        PresetHelpers::YAML_Helpers::ResolveMergeKeys(old);
        Require(Equal(old, Expand(text)));
    }
    Reject("templates: []", "must be a mapping");
    Reject("templates: {v: 1, v: 2}", "Duplicate YAML template");
    Reject("templates: {'bad name': 1}", "Invalid template name");
    Reject("templates: {v: $}\nx: v(1)", "Invalid parameter");
    Reject("templates: {v: $x}\nx: v()", "expects 1");
    Reject("templates: {v: 1}\nx: v(2)", "expects 0");
    Reject("templates: {v: 1}\nx: v(", "Missing ')'");
    Reject("templates: {v: v()}\nx: v()", "Recursive");
    Reject("templates: {a: b(), b: a()}\nx: a()", "Recursive");
    Reject("templates: {}\nx: &x [*x]", "Circular YAML alias");
    Reject("templates: {v: {<<: $x}}\nx: v(4)", "YAML merge requires");
    Reject("templates: {v: &cycle [*cycle]}\nx: v()", "Circular YAML alias");
    Reject("templates: {}\ntemplates: {}", "Duplicate top-level 'templates'");
    Reject("&root {<<: *root}", "Circular YAML alias");
    auto priority = Expand(R"yaml(
first: &first {templates: {v: first}}
second: &second {templates: {v: second}}
<<: [*first, *second]
x: v()
)yaml");
    Require(priority["x"].Scalar() == "first");
    Require(Expand("<<: {templates: {v: inherited}}\ntemplates: {v: explicit}\nx: v()")["x"].Scalar() == "explicit");
    auto sharedCall = Expand("templates: {v: {x: $x}}\na: &call v(2)\nb: *call");
    Require(sharedCall["a"].is(sharedCall["b"]));
    Require(sharedCall["b"]["x"].as<int>() == 2);
    Require(Expand("templates: {v: $x}\nx: v(!!str 'v(7)')")["x"].Scalar() == "v(7)");
    Reject("templates: {v: $x}\nx: v([1, 2)", "end of sequence");
    try {
        auto bad = YAML::Load("templates: []");
        PresetHelpers::YAML_Helpers::ResolveTemplates(bad, "example.yml");
        Require(false);
    } catch (const YAML::Exception& error) {
        Require(std::string(error.what()).find("example.yml") != std::string::npos);
    }
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
