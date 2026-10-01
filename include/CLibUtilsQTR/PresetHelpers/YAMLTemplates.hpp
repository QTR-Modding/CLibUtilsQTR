#pragma once
#include "YAMLMerge.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace PresetHelpers::YAML_Helpers {
    namespace detail {
        class TemplateExpander {
            struct Definition {
                YAML::Node body;
                std::vector<std::string> parameters;
            };
            struct Call {
                std::string name;
                YAML::Node arguments;
            };
            using Arguments = std::unordered_map<std::string, YAML::Node>;
            YAML::Node templateSection;
            std::unordered_map<std::string, Definition> definitions;
            std::vector<std::string> activeCalls;
            std::unordered_map<std::string, Call> parsedCalls;

            static void Fail(const YAML::Node& node, const std::string& message) {
                throw YAML::RepresentationException(node.Mark(), message);
            }

            static bool IsName(const std::string& name) {
                return !name.empty() && name.find_first_not_of(
                    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
            }

            std::optional<Call> ParseCall(const YAML::Node& node, bool cacheDefinitionCall = false) {
                if (!node.IsScalar() || node.Tag() == "tag:yaml.org,2002:str") return std::nullopt;
                const auto& text = node.Scalar();
                const auto open = text.find('(');
                if (open == std::string::npos || !definitions.contains(text.substr(0, open))) return std::nullopt;
                if (text.back() != ')') Fail(node, "Missing ')' in template call");
                if (const auto cached = parsedCalls.find(text); cached != parsedCalls.end()) {
                    return Call{cached->second.name, YAML::Clone(cached->second.arguments)};
                }
                auto args = YAML::Load("[" + text.substr(open + 1, text.size() - open - 2) + "]");
                Call call{text.substr(0, open), args};
                if (cacheDefinitionCall) parsedCalls.emplace(text, call);
                return call;
            }

            void FindParameters(const YAML::Node& node, std::vector<std::string>& parameters,
                                std::vector<std::pair<YAML::Node, bool>>& visited) {
                const auto seen = std::find_if(visited.begin(), visited.end(), [&](const auto& entry) { return entry.first.is(node); });
                if (seen != visited.end()) {
                    if (!seen->second) Fail(node, "Circular YAML alias");
                    return;
                }
                const auto index = visited.size();
                visited.emplace_back(node, false);
                if (const auto call = ParseCall(node, true)) {
                    for (const auto& arg : call->arguments) FindParameters(arg, parameters, visited);
                } else if (node.IsScalar()) {
                    const auto& value = node.Scalar();
                    if (value.starts_with('$') && !value.starts_with("$$")) {
                        const auto name = value.substr(1);
                        if (!IsName(name)) Fail(node, "Invalid parameter '" + value + "'");
                        if (std::find(parameters.begin(), parameters.end(), name) == parameters.end()) parameters.push_back(name);
                    }
                } else if (node.IsSequence()) {
                    for (const auto& child : node) FindParameters(child, parameters, visited);
                } else if (node.IsMap()) {
                    for (const auto& entry : node) FindParameters(entry.second, parameters, visited);
                }
                visited[index].second = true;
            }

            void Substitute(YAML::Node node, const Arguments& arguments, std::vector<YAML::Node>& visited) {
                if (node.IsNull() || (node.IsScalar() && !node.Scalar().starts_with('$') &&
                    (node.Tag() == "tag:yaml.org,2002:str" || node.Scalar().find('(') == std::string::npos))) return;
                if (std::any_of(visited.begin(), visited.end(), [&](const auto& seen) { return seen.is(node); })) return;
                visited.push_back(node);
                if (auto call = ParseCall(node)) {
                    for (auto arg : call->arguments) Substitute(arg, arguments, visited);
                    node = ExpandCall(*call, node);
                } else if (node.IsScalar()) {
                    const auto text = node.Scalar();
                    if (text.starts_with("$$")) {
                        node = text.substr(1);
                    } else if (text.starts_with('$')) {
                        const auto found = arguments.find(text.substr(1));
                        if (found == arguments.end()) Fail(node, "Unknown parameter '" + text + "'");
                        node = YAML::Clone(found->second);
                    }
                } else if (node.IsSequence()) {
                    for (auto child : node) Substitute(child, arguments, visited);
                } else if (node.IsMap()) {
                    for (const auto& entry : node) Substitute(entry.second, arguments, visited);
                }
            }

            YAML::Node ExpandCall(Call& call, const YAML::Node& source) {
                const auto& definition = definitions.at(call.name);
                if (call.arguments.size() != definition.parameters.size()) {
                    Fail(source, "Template '" + call.name + "' expects " + std::to_string(definition.parameters.size()) + " arguments");
                }
                if (std::find(activeCalls.begin(), activeCalls.end(), call.name) != activeCalls.end()) {
                    Fail(source, "Recursive YAML template call to '" + call.name + "'");
                }
                // Evaluate arguments before marking this body's expansion active.
                Expand(call.arguments);
                Arguments arguments;
                for (std::size_t i = 0; i < definition.parameters.size(); ++i) {
                    arguments.emplace(definition.parameters[i], call.arguments[i]);
                }
                activeCalls.push_back(call.name);
                auto result = YAML::Clone(definition.body);
                std::vector<YAML::Node> visited;
                Substitute(result, arguments, visited);
                Expand(result);
                activeCalls.pop_back();
                return result;
            }

            void Expand(YAML::Node node, std::vector<std::pair<YAML::Node, bool>>& visited) {
                if (node.IsNull() || (node.IsScalar() &&
                    (node.Tag() == "tag:yaml.org,2002:str" || node.Scalar().find('(') == std::string::npos))) return;
                if (node.is(templateSection)) return;
                const auto found = std::find_if(visited.begin(), visited.end(), [&](const auto& entry) { return entry.first.is(node); });
                if (found != visited.end()) {
                    if (!found->second) Fail(node, "Circular YAML alias");
                    return;
                }
                const auto index = visited.size();
                visited.emplace_back(node, false);
                if (auto call = ParseCall(node)) {
                    node = ExpandCall(*call, node);
                } else if (node.IsSequence()) {
                    for (auto child : node) Expand(child, visited);
                } else if (node.IsMap()) {
                    for (const auto& entry : node) Expand(entry.second, visited);
                    MergeMapping(node);
                }
                visited[index].second = true;
            }

        public:
            explicit TemplateExpander(const YAML::Node& templates) : templateSection(templates) {
                if (!templates.IsMap()) Fail(templates, "YAML 'templates' must be a mapping");
                for (const auto& entry : templates) {
                    if (!entry.first.IsScalar() || !IsName(entry.first.Scalar())) Fail(entry.first, "Invalid template name");
                    if (!definitions.emplace(entry.first.Scalar(), Definition{entry.second, {}}).second) {
                        Fail(entry.first, "Duplicate YAML template '" + entry.first.Scalar() + "'");
                    }
                }
                for (auto& [name, definition] : definitions) {
                    std::vector<std::pair<YAML::Node, bool>> visited;
                    FindParameters(definition.body, definition.parameters, visited);
                }
            }

            void Expand(YAML::Node node) {
                std::vector<std::pair<YAML::Node, bool>> visited;
                Expand(node, visited);
            }
        };
    }

    // Expands document-local calls and YAML merges before config parsing.
    // https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Configuration-and-Strings#parameterized-yaml-templates
    inline void ResolveTemplates(YAML::Node document, const std::string& source = {}) try {
        if (!document.IsMap()) {
            ResolveMergeKeys(document);
            return;
        }
        bool foundTemplates = false;
        for (const auto& entry : document) {
            if (!entry.first.IsScalar() || entry.first.Scalar() != "templates") continue;
            if (foundTemplates) throw YAML::RepresentationException(entry.first.Mark(), "Duplicate top-level 'templates' section");
            foundTemplates = true;
        }
        if (!foundTemplates) {
            std::unordered_map<int, std::vector<std::pair<YAML::Node, bool>>> visited;
            detail::ResolveMergeKeys(document, visited, false);
        }
        if (!std::as_const(document)["templates"].IsDefined()) {
            ResolveMergeKeys(document);
            return;
        }
        const auto templates = std::as_const(document)["templates"];
        detail::TemplateExpander expander(templates);
        document.remove("templates");
        expander.Expand(document);
    } catch (const YAML::Exception& error) {
        if (source.empty()) throw;
        throw YAML::RepresentationException(error.mark, source + ": " + error.msg);
    }
}
