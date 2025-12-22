#pragma once
#include "CLibUtilsQTR/StringHelpers.hpp"
#include "CLibUtilsQTR/PresetHelpers/PresetHelpers.hpp"

namespace PresetHelpers::TXT_Helpers {
    inline void GatherForms(const std::string& folder_path) {
        if (!std::filesystem::exists(folder_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
            const auto filename = entry.path().string();

            std::ifstream file(filename);
            if (!file.is_open()) {
                continue;
            }

            const auto file_name_without_extension = entry.path().stem().string();

            formGroups[file_name_without_extension] = {};

            std::string line;
            while (std::getline(file, line)) {
                // trim whitespace from the line
                line = StringHelpers::trim(line);
                if (line.empty()) continue; // skip empty lines
                if (auto a_form = FormReader::GetFormEditorIDFromString(line); a_form > 0) {
                    std::unique_lock lock(formGroups_mutex_);
                    formGroups.at(file_name_without_extension).insert(a_form);
                }
            }
        }
    }
}