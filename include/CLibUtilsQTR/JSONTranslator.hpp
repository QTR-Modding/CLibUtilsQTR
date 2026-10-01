#pragma once

#include "Translator.hpp"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace clib_utilsQTR {
    class JSONTranslator final : public Translator {
    public:
        using Translator::Translator;

        bool Load(const std::filesystem::path& path) {
            if (StringHelpers::toLowercase(path.extension().string()) != ".json") return Translator::Load(path);
            return LoadFile(path, ".json", [this](const auto& text, const auto& location) {
                return LoadJSON(text, location);
            });
        }

    private:
        bool LoadJSON(const std::string& text, std::string_view path) {
            rapidjson::Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>(text.data(), text.size());
            if (document.HasParseError())
                return Report(path, std::format("JSON byte {}: {}", document.GetErrorOffset(), rapidjson::GetParseError_En(document.GetParseError())));
            if (!document.IsObject()) return Report(path, "expected a JSON object of translation strings");
            bool success = true;
            for (const auto& entry : document.GetObject()) {
                const std::string key(entry.name.GetString(), entry.name.GetStringLength());
                const auto location = std::format("{} [{}]", path, key);
                if (!entry.value.IsString()) {
                    Report(location, "translation must be a string");
                    success = false;
                    continue;
                }
                if (!Insert(key, std::string(entry.value.GetString(), entry.value.GetStringLength()), location)) success = false;
            }
            return success;
        }
    };
}
