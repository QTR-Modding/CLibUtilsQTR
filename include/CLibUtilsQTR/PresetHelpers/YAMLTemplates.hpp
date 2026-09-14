#pragma once
#include "YAMLMerge.hpp"
#include <string>
#include <unordered_map>

namespace PresetHelpers::YAML_Helpers {
    namespace detail {
        class TemplateExpander {
            struct Definition {
                std::vector<std::string> parameters;
                YAML::Node body;
            };
            using Arguments = std::unordered_map<std::string, YAML::Node>;
            std::unordered_map<std::string, Definition> definitions;
            std::vector<std::string> activeCalls;

            static void Fail(const YAML::Node& node, const std::string& message) {
                throw YAML::RepresentationException(node.Mark(), message);
            }

            static void CheckKeys(const YAML::Node& node, const char* first, const char* second) {
                bool seenFirst = false;
                bool seenSecond = false;
                for (const auto& entry : node) {
                    if (!entry.first.IsScalar() ||
                        (entry.first.Scalar() != first && entry.first.Scalar() != second)) {
                        Fail(entry.first, "Expected only '" + std::string(first) + "' and '" + second + "'");
                    }
                    auto& seen = entry.first.Scalar() == first ? seenFirst : seenSecond;
                    if (seen) Fail(entry.first, "Duplicate field '" + entry.first.Scalar() + "'");
                    seen = true;
                }
            }

            void Substitute(YAML::Node node, const Arguments& arguments, const std::string& name,
                            std::vector<YAML::Node>& visited) {
                if (std::any_of(visited.begin(), visited.end(), [&](const auto& seen) { return seen.is(node); })) return;
                visited.push_back(node);
                if (node.IsScalar()) {
                    const auto text = node.Scalar();
                    if (text.starts_with("$$")) {
                        const auto tag = node.Tag();
                        node = text.substr(1);
                        node.SetTag(tag);
                    } else if (text.starts_with('$')) {
                        const auto found = arguments.find(text.substr(1));
                        if (found == arguments.end()) Fail(node, "Unknown parameter '" + text + "' in template '" + name + "'");
                        node = YAML::Clone(found->second);
                    }
                } else if (node.IsSequence()) {
                    for (auto child : node) Substitute(child, arguments, name, visited);
                } else if (node.IsMap()) {
                    for (const auto& entry : node) Substitute(entry.second, arguments, name, visited);
                }
            }

            YAML::Node ExpandCall(const YAML::Node& call, std::vector<std::pair<YAML::Node, bool>>& visited) {
                CheckKeys(call, "use", "args");
                if (!call["use"].IsScalar()) Fail(call, "Template 'use' must be a name");
                const auto name = call["use"].Scalar();
                const auto found = definitions.find(name);
                if (found == definitions.end()) Fail(call, "Unknown YAML template '" + name + "'");
                if (std::find(activeCalls.begin(), activeCalls.end(), name) != activeCalls.end()) {
                    Fail(call, "Recursive YAML template call to '" + name + "'");
                }
                const auto args = call["args"];
                const auto& definition = found->second;
                if (!args.IsDefined() || !args.IsSequence() || args.size() != definition.parameters.size()) {
                    Fail(call, "Template '" + name + "' expects " + std::to_string(definition.parameters.size()) + " arguments in 'args'");
                }
                Arguments arguments;
                for (std::size_t i = 0; i < definition.parameters.size(); ++i) {
                    Expand(args[i], visited);
                    arguments.emplace(definition.parameters[i], args[i]);
                }
                activeCalls.push_back(name);
                auto result = YAML::Clone(definition.body);
                std::vector<YAML::Node> substituted;
                Substitute(result, arguments, name, substituted);
                PresetHelpers::YAML_Helpers::ResolveMergeKeys(result);
                Expand(result);
                activeCalls.pop_back();
                return result;
            }

        public:
            explicit TemplateExpander(const YAML::Node& templates) {
                if (!templates.IsMap()) Fail(templates, "YAML 'templates' must be a mapping");
                for (const auto& entry : templates) {
                    if (!entry.first.IsScalar() || entry.first.Scalar().empty()) Fail(entry.first, "A YAML template needs a nonempty name");
                    const auto name = entry.first.Scalar();
                    const auto& node = entry.second;
                    if (!node.IsMap()) Fail(node, "Template '" + name + "' must be a mapping");
                    CheckKeys(node, "parameters", "body");
                    if (!node["parameters"].IsDefined() || !node["parameters"].IsSequence() || !node["body"].IsDefined()) {
                        Fail(node, "Template '" + name + "' requires 'parameters' and 'body'");
                    }
                    Definition definition;
                    definition.body.reset(node["body"]);
                    for (const auto& parameter : node["parameters"]) {
                        if (!parameter.IsScalar() || parameter.Scalar().empty() || parameter.Scalar().starts_with('$')) {
                            Fail(parameter, "Invalid parameter name in template '" + name + "'");
                        }
                        const auto& parameterName = parameter.Scalar();
                        if (std::find(definition.parameters.begin(), definition.parameters.end(), parameterName) != definition.parameters.end()) {
                            Fail(parameter, "Duplicate parameter '" + parameterName + "' in template '" + name + "'");
                        }
                        definition.parameters.push_back(parameterName);
                    }
                    if (!definitions.emplace(name, std::move(definition)).second) Fail(entry.first, "Duplicate YAML template '" + name + "'");
                }
            }

            void Expand(YAML::Node node, std::vector<std::pair<YAML::Node, bool>>& visited) {
                const auto found = std::find_if(visited.begin(), visited.end(), [&](const auto& entry) { return entry.first.is(node); });
                if (found != visited.end()) {
                    if (!found->second) Fail(node, "Circular YAML alias");
                    return;
                }
                const auto index = visited.size();
                visited.emplace_back(node, false);
                if (node.IsMap() && std::as_const(node)["use"].IsDefined()) {
                    node = ExpandCall(node, visited);
                } else if (node.IsSequence()) {
                    for (auto child : node) Expand(child, visited);
                } else if (node.IsMap()) {
                    for (const auto& entry : node) Expand(entry.second, visited);
                }
                visited[index].second = true;
            }

            void Expand(YAML::Node node) {
                std::vector<std::pair<YAML::Node, bool>> visited;
                Expand(node, visited);
            }
        };
    }

    // Resolves merge keys and opt-in, document-local templates in place before config parsing.
    // Throws YAML::Exception on invalid input; discard the document on failure.
    // https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Configuration-and-Strings#parameterized-yaml-templates
    inline void ResolveTemplates(YAML::Node document, const std::string& source = {}) try {
        if (!document.IsMap() || !std::as_const(document)["templates"].IsDefined()) {
            ResolveMergeKeys(document);
            return;
        }
        auto result = YAML::Clone(document);
        detail::TemplateExpander expander(std::as_const(result)["templates"]);
        result.remove("templates");
        ResolveMergeKeys(result);
        expander.Expand(result);
        document = result;
    } catch (const YAML::Exception& error) {
        if (source.empty()) throw;
        throw YAML::RepresentationException(error.mark, source + ": " + error.msg);
    }
}
