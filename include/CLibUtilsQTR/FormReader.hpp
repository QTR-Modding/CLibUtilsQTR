#pragma once
#include <ranges>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <cctype>
#include <ios>
#include <sstream>
#include "ClibUtil/editorID.hpp"

namespace FormReader {
    using FormID = RE::FormID;

    // Global masters list
    const std::vector<std::string> masters = {"00", "01", "02", "03", "04"};

    // Function to clean the input string
    inline std::string clean(const std::string& input) {
        // Remove all non-alphanumeric characters
        std::string s = std::regex_replace(input, std::regex("[^a-zA-Z0-9]+"), "");

        // Convert to uppercase
        std::ranges::transform(s.begin(), s.end(), s.begin(), toupper);

        // Trim whitespace from both ends
        s.erase(s.begin(), std::ranges::find_if(s.begin(), s.end(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::ranges::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());

        return s;
    }

    // Struct to hold the result
    struct Result {
        std::string output;
        bool error;
        bool isMod;
    };

    // Function to process the input and produce the output
    inline Result processInput(const std::string& input, const std::string& name) {
        Result res;
        res.output = "";
        res.error = true;
        res.isMod = true;

        if (!input.empty()) {
            std::string result = clean(input);
            std::string modName = name;

            // Trim modName
            modName.erase(modName.begin(), std::find_if(modName.begin(), modName.end(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }));
            modName.erase(std::find_if(modName.rbegin(), modName.rend(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }).base(), modName.end());

            if (result.length() != 8) {
                res.output += "input must be 8 characters long";
            }

            if (res.output.empty()) {
                res.error = false;

                std::string firstTwoDigits = result.substr(0, 2);

                bool esl = false;
                bool mod = true;

                if (firstTwoDigits == "FE") {
                    esl = true;
                } else if (std::ranges::find(masters, firstTwoDigits) != masters.end()) {
                    mod = false;
                }

                res.isMod = mod;

                if (mod) {
                    if (esl) {
                        result = result.substr(5);
                    } else {
                        result = result.substr(2);
                    }
                }

                if (mod && modName.empty()) {
                    res.error = true;
                    res.output = "Mod name required for non base game files";
                } else {
                    size_t i = 0;
                    while (i < result.length() && result[i] == '0') {
                        i++;
                    }

                    res.output = "0x" + result.substr(i) + (mod ? ("~" + name) : "");
                }
            }
        }

        return res;
    }


    inline std::vector<std::string> split(const std::string& a_str, std::string_view a_delimiter) {
        auto range = a_str | std::ranges::views::split(a_delimiter) |
                     std::ranges::views::transform([](auto&& r) { return std::string_view(r); });
        return {range.begin(), range.end()};
    }


    inline FormID GetForm(const char* fileName, const uint32_t localId) {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        const auto formId = dataHandler->LookupFormID(localId, fileName);
        return formId;
    }

    inline FormID GetFormIDFromString(const std::string& input) {
        FormID form_id_;
        std::stringstream ss;
        ss << std::hex << input;
        ss >> form_id_;
        return form_id_;
    }

    inline bool isValidHexWithLength7or8(const char* input) {
        std::string inputStr(input);

        if (inputStr.substr(0, 2) == "0x") {
            // Remove "0x" from the beginning of the string
            inputStr = inputStr.substr(2);
        }

        const std::regex hexRegex("^[0-9A-Fa-f]{7,8}$"); // Allow 7 to 8 characters
        const bool isValid = std::regex_match(inputStr, hexRegex);
        return isValid;
    }

    inline RE::TESForm* GetFormByID(const RE::FormID id, const std::string& editor_id = "") {
        if (!editor_id.empty()) {
            if (auto* form = RE::TESForm::LookupByEditorID(editor_id)) return form;
        }
        if (id > 0) {
            if (const auto form = RE::TESForm::LookupByID(id)) return form;
        }
        return nullptr;
    }

    template <typename T>
    T* GetFormByID(const RE::FormID id, const std::string& editor_id = "") {
        if (!editor_id.empty()) {
            if (auto* form = RE::TESForm::LookupByEditorID<T>(editor_id)) return form;
        }
        if (id > 0) {
            if (const auto form = RE::TESForm::LookupByID<T>(id)) return form;
        }
        return nullptr;
    }

    inline RE::TESForm* GetFormFromString(const std::string& formEditorId) {
        if (formEditorId.empty()) return nullptr;

        static std::string delimiter = "~";
        const auto plugin_and_localid = split(formEditorId, delimiter);
        if (plugin_and_localid.size() == 2) {
            const auto& plugin_name = plugin_and_localid[1];
            const auto local_id = FormReader::GetFormIDFromString(plugin_and_localid[0]);
            const auto formid = FormReader::GetForm(plugin_name.c_str(), local_id);
            if (const auto form = RE::TESForm::LookupByID(formid)) return form;
        }

        if (isValidHexWithLength7or8(formEditorId.c_str())) {
            if (const auto temp_form = GetFormByID(FormReader::GetFormIDFromString(formEditorId)))
                return temp_form;
        }

        if (const auto temp_form = GetFormByID(0, formEditorId)) return temp_form;

        return nullptr;
    }

    inline FormID GetFormEditorIDFromString(const std::string& formEditorId) {
        if (auto a_form = GetFormFromString(formEditorId)) {
            return a_form->formID;
        }
        return 0;
    }

    inline std::string GetEditorID(FormID a_formid) {
        if (auto a_form = GetFormByID(a_formid)) {
            return clib_util::editorID::get_editorID(a_form);
        }
        return "";
    }
}