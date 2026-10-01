#pragma once
#include <shared_mutex>
#include "CLibUtilsQTR/StringHelpers.hpp"
#include "CLibUtilsQTR/PresetHelpers/PresetHelpers.hpp"
#include "CLibUtilsQTR/FormReader.hpp"
#include "CLibUtilsQTR/PresetHelpers/YAMLTemplates.hpp"

namespace PresetHelpers::YAML_Helpers {
    inline std::vector<FormID> StringToFormIDs(const std::string& input) {
        {
            std::shared_lock lock(formGroups_mutex_);
            if (const auto found = formGroups.find(input); found != formGroups.end()) {
                return {found->second.begin(), found->second.end()};
            }
        }

        if (FormID a_formid = FormReader::GetFormEditorIDFromString(input); a_formid > 0) {
            return {a_formid};
        }
        return {};
    }

    template <typename T>
    std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key) {
        const auto field = node[key];
        auto res = std::vector<T>{};
        if (field.IsScalar()) {
            res.push_back(field.as<T>());
        } else {
            res.reserve(field.size());
            for (const auto& value : field) {
                res.push_back(value.as<T>());
            }
        }
        return res;
    }

    template <typename T, typename U>
    std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key);

    template <>
    inline std::vector<FormID> CollectFrom<FormID, std::string>(const YAML::Node& node, const std::string& key) {
        const auto field = node[key];
        if (field.IsScalar()) return StringToFormIDs(field.as<std::string>());
        auto res = std::vector<FormID>{};
        res.reserve(field.size());
        for (const auto& value : field) {
            auto temp = StringToFormIDs(value.as<std::string>());
            res.insert(res.end(), temp.begin(), temp.end());
        }
        return res;
    }
}