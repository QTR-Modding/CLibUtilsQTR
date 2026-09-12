#pragma once
#include <algorithm>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace PresetHelpers::YAML_Helpers {
    namespace detail {
        inline bool IsMergeKey(const YAML::Node& key) {
            return key.IsScalar() && (key.Tag() == "tag:yaml.org,2002:merge" ||
                (key.Scalar() == "<<" && key.Tag() == "?"));
        }

        inline void ResolveMergeKeys(YAML::Node node, std::vector<std::pair<YAML::Node, bool>>& visited) {
            if (!node.IsMap() && !node.IsSequence()) return;
            const auto found = std::find_if(visited.begin(), visited.end(), [&](const auto& entry) {
                return entry.first.is(node);
            });
            if (found != visited.end()) {
                if (!found->second) throw YAML::RepresentationException(node.Mark(), "Circular YAML alias");
                return;
            }
            const auto index = visited.size();
            visited.emplace_back(node, false);
            if (node.IsSequence()) {
                for (auto child : node) ResolveMergeKeys(child, visited);
            } else {
                std::vector<YAML::Node> sources;
                bool hasMerge = false;
                for (const auto& entry : node) {
                    ResolveMergeKeys(entry.second, visited);
                    if (!IsMergeKey(entry.first)) continue;
                    if (hasMerge) throw YAML::RepresentationException(entry.first.Mark(), "Use a sequence for multiple YAML merge sources");
                    hasMerge = true;
                    if (entry.second.IsMap()) sources.push_back(entry.second);
                    else if (entry.second.IsSequence()) {
                        for (auto source : entry.second) sources.push_back(source);
                    } else {
                        throw YAML::RepresentationException(entry.second.Mark(), "YAML merge requires a map or sequence of maps");
                    }
                }
                for (const auto& source : sources) {
                    if (!source.IsMap()) throw YAML::RepresentationException(source.Mark(), "YAML merge requires maps");
                    for (const auto& entry : source) {
                        if (!entry.first.IsScalar()) throw YAML::RepresentationException(entry.first.Mark(), "YAML merge requires scalar keys");
                        const auto key = entry.first.Scalar();
                        if (!std::as_const(node)[key].IsDefined()) node[key] = entry.second;
                    }
                }
                // Quoted "<<" is an ordinary key, not a merge directive.
                for (auto it = node.begin(); it != node.end(); ++it) {
                    if (IsMergeKey(it->first)) {
                        node.remove(it->first);
                        break;
                    }
                }
            }
            visited[index].second = true;
        }
    }

    // Expands merges in place. Explicit keys win; earlier merge sources win.
    // Throws YAML::Exception for invalid merges or cyclic aliases; discard the document on failure.
    // https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Configuration-and-Strings
    inline void ResolveMergeKeys(YAML::Node node) {
        std::vector<std::pair<YAML::Node, bool>> visited;
        detail::ResolveMergeKeys(node, visited);
    }
}
