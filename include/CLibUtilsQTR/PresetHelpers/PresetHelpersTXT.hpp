#pragma once
#include <fstream>
#include <mutex>
#include <optional>
#include "CLibUtilsQTR/StringHelpers.hpp"
#include "CLibUtilsQTR/PresetHelpers/PresetHelpers.hpp"

namespace PresetHelpers::TXT_Helpers {
    inline std::optional<std::unordered_set<RE::TESForm*>> LoadFormGroup(
        const std::filesystem::path& folder, const std::filesystem::path& name) {
        if (!name.has_filename() || name != name.filename()) {
            return std::nullopt;
        }
        auto filename = name;
        filename += ".txt";
        std::ifstream file(folder / filename);
        if (!file.is_open()) {
            return std::nullopt;
        }

        std::unordered_set<RE::TESForm*> forms;
        for (std::string line; std::getline(file, line);) {
            const auto form = FormReader::GetFormFromString(StringHelpers::trim(line));
            if (form && form->GetFormID()) {
                forms.insert(form);
            }
        }
        return forms;
    }

    inline void GatherForms(const std::string& folder_path) {
        if (!std::filesystem::exists(folder_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".txt") {
                continue;
            }
            const auto name = entry.path().stem();
            const auto forms = LoadFormGroup(folder_path, name);
            if (!forms) {
                continue;
            }
            std::unique_lock lock(formGroups_mutex_);
            auto& formIDs = formGroups[name.string()];
            formIDs.clear();
            for (const auto form : *forms) {
                formIDs.insert(form->GetFormID());
            }
        }
    }
}
