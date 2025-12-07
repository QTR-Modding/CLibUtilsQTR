#pragma once
#include <filesystem>
#include <shared_mutex>
#include "CLibUtilsQTR/StringHelpers.hpp"
#include "CLibUtilsQTR/PresetHelpers/PresetHelpers.hpp"
#include "CLibUtilsQTR/FormReader.hpp"
#include "yaml-cpp/yaml.h"

namespace PresetHelpers::YAML_Helpers {
    inline std::vector<FormID> StringToFormIDs(const std::string& input) {
        if (std::shared_lock lock(formGroups_mutex_);
            formGroups.contains(input)) {
            auto& temp = formGroups.at(input);
            return std::vector<FormID>(temp.begin(), temp.end());
        }

        if (FormID a_formid = FormReader::GetFormEditorIDFromString(input); a_formid > 0) {
            return {a_formid};
        }
        return {};
    }

    template <typename T>
    std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key) {
        auto res = std::vector<T>{};
        if (node[key].IsScalar()) {
            res.push_back(node[key].as<T>());
        } else {
            for (const auto& value : node[key]) {
                res.push_back(value.as<T>());
            }
        }
        return res;
    }

    template <typename T, typename U>
    std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key);

    template <>
    inline std::vector<FormID> CollectFrom<FormID, std::string>(const YAML::Node& node, const std::string& key) {
        auto res = std::vector<FormID>{};
        if (node[key].IsScalar()) {
            auto temp = StringToFormIDs(node[key].as<std::string>());
            res.insert(res.end(), temp.begin(), temp.end());
        } else {
            for (const auto& iterator_value : node[key]) {
                auto temp = StringToFormIDs(iterator_value.as<std::string>());
                res.insert(res.end(), temp.begin(), temp.end());
            }
        }
        return res;
    }
}