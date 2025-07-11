#pragma once
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include "CLibUtilsQTR/StringHelpers.hpp"
#include "CLibUtilsQTR/FormReader.hpp"
#include "yaml-cpp/yaml.h"

namespace PresetHelpers {

	using FormID = FormReader::FormID;

	inline std::shared_mutex formGroups_mutex_;
    inline std::unordered_map<std::string, std::unordered_set<FormID>> formGroups;

	namespace TXT_Helpers {
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

                std::string line;
                while (std::getline(file, line)) {
                    // trim whitespace from the line
                    line = StringHelpers::trim(line);
                    if (line.empty()) continue; // skip empty lines
			        if (auto a_form = FormReader::GetFormEditorIDFromString(line); a_form>0) {
						std::unique_lock lock(formGroups_mutex_);
						formGroups[file_name_without_extension].insert(a_form);
					}
		        }
	        }
		}
	    
	}

	namespace YAML_Helpers{
        inline std::vector<FormID> StringToFormIDs(const std::string& input) {

			if (std::shared_lock lock(formGroups_mutex_); 
				formGroups.contains(input)) {
				auto& temp = formGroups.at(input);
				return std::vector<FormID>(temp.begin(), temp.end());
			}

		    if (FormID a_formid = FormReader::GetFormEditorIDFromString(input); a_formid > 0) {
                return { a_formid };
	        }
            return {};
		}

		template <typename T>
		std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key)
		{
			auto res = std::vector<T>{};
			if (node[key].IsScalar()) {
				res.push_back(node[key].as<T>());
			}
			else {
				for (const auto& value : node[key]) {
					res.push_back(value.as<std::uint8_t>());
				}
			}
			return res;
		}

		template <typename T, typename U>
		std::vector<T> CollectFrom(const YAML::Node& node, const std::string& key);

		template <>
		inline std::vector<FormID> CollectFrom<FormID,std::string>(const YAML::Node& node, const std::string& key) {
			auto res = std::vector<FormID>{};
			if (node[key].IsScalar()) {
				auto temp = StringToFormIDs(node[key].as<std::string>());
				res.insert(res.end(), temp.begin(), temp.end());
			}
			else {
				for (const auto& iterator_value : node[key]) {
					auto temp = StringToFormIDs(iterator_value.as<std::string>());
					res.insert(res.end(), temp.begin(), temp.end());
				}
			}
			return res;
		}
	}
	
} // namespace PresetHelpers