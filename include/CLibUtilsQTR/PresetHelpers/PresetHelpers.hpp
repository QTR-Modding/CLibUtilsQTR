#pragma once
#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include "CLibUtilsQTR/FormReader.hpp"

namespace PresetHelpers {
    using FormID = FormReader::FormID;

    inline std::shared_mutex formGroups_mutex_;
    inline std::unordered_map<std::string, std::unordered_set<FormID>> formGroups;
} // namespace PresetHelpers