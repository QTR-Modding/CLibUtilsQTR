#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/stringbuffer.h>

#include "StringHelpers.hpp"

namespace clib_utilsQTR {
    // Load before readers start, or synchronize reloads externally.
    class Translator {
    public:
        using Table = std::map<std::string, std::string, std::less<>>;
        using ErrorHandler = std::function<void(std::string_view)>;

        explicit Translator(Table defaults, ErrorHandler onError = {})
            : defaults_(std::move(defaults)), onError_(std::move(onError)) {}

        // Replaces previous translations even on failure. False means at least one error;
        // valid entries from a partially valid file remain available.
        bool Load(const std::filesystem::path& path) {
            translations_.clear();
            std::ifstream file(path, std::ios::binary);
            if (!file) return Report(path.string(), "cannot open translation file");
            const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            if (file.bad()) return Report(path.string(), "cannot read translation file");
            std::string text;
            if (!Decode(bytes, text)) return Report(path.string(), "invalid text encoding (expected UTF-8 or BOM-marked UTF-16LE)");
            const auto extension = StringHelpers::toLowercase(path.extension().string());
            if (extension == ".txt") return LoadTXT(text, path.string());
            if (extension == ".json") return LoadJSON(text, path.string());
            return Report(path.string(), "unsupported translation file extension");
        }

        // The view belongs to this object, or to key when neither table contains it.
        [[nodiscard]] std::string_view Get(std::string_view key) const {
            if (const auto it = translations_.find(key); it != translations_.end()) return it->second;
            return GetDefault(key);
        }

        template <class... Args>
        [[nodiscard]] std::string Format(std::string_view key, Args&&... args) const {
            try {
                return std::vformat(Get(key), std::make_format_args(args...));
            } catch (const std::format_error& error) {
                Report(key, error.what());
            }
            const auto fallback = GetDefault(key);
            try {
                return std::vformat(fallback, std::make_format_args(args...));
            } catch (const std::format_error& error) {
                Report(key, error.what());
                return std::string(fallback);
            }
        }

    private:
        [[nodiscard]] std::string_view GetDefault(std::string_view key) const {
            const auto it = defaults_.find(key);
            return it != defaults_.end() ? std::string_view(it->second) : key;
        }

        bool Report(std::string_view location, std::string_view message) const {
            if (onError_) onError_(std::format("{}: {}", location, message));
            return false;
        }

        template <class Encoding, class Stream>
        static bool Transcode(Stream& stream, std::string& output) {
            rapidjson::StringBuffer buffer;
            while (stream.Peek()) {
                if (!rapidjson::Transcoder<Encoding, rapidjson::UTF8<>>::Validate(stream, buffer)) return false;
            }
            output.assign(buffer.GetString(), buffer.GetSize());
            return true;
        }

        static bool Decode(const std::string& bytes, std::string& output) {
            constexpr std::string_view utf16BOM = "\xFF\xFE";
            constexpr std::string_view utf8BOM = "\xEF\xBB\xBF";
            if (bytes.starts_with(utf16BOM)) {
                constexpr std::size_t codeUnitBytes = sizeof(char16_t);
                constexpr unsigned byteBits = 8;
                if (bytes.size() % codeUnitBytes != 0) return false;
                std::u16string units;
                for (auto i = utf16BOM.size(); i < bytes.size(); i += codeUnitBytes) {
                    const auto unit = static_cast<char16_t>(static_cast<unsigned char>(bytes[i]) |
                        (static_cast<unsigned char>(bytes[i + 1]) << byteBits));
                    if (unit == u'\0') return false;
                    units.push_back(unit);
                }
                rapidjson::GenericStringStream<rapidjson::UTF16<char16_t>> stream(units.c_str());
                return Transcode<rapidjson::UTF16<char16_t>>(stream, output);
            }
            if (bytes.find('\0') != std::string::npos) return false;
            const auto offset = bytes.starts_with(utf8BOM) ? utf8BOM.size() : 0;
            rapidjson::MemoryStream stream(bytes.data() + offset, bytes.size() - offset);
            return Transcode<rapidjson::UTF8<>>(stream, output);
        }

        static std::string UnescapeTXT(std::string_view value) {
            std::string result;
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '\\' && i + 1 < value.size()) {
                    switch (value[i + 1]) {
                    case 'n': result += '\n'; ++i; continue;
                    case 't': result += '\t'; ++i; continue;
                    case '\\': result += '\\'; ++i; continue;
                    default: break;
                    }
                }
                result += value[i];
            }
            return result;
        }

        bool Insert(std::string key, std::string value, std::string_view location) {
            if (key.empty() || key.find('\0') != std::string::npos || value.find('\0') != std::string::npos)
                return Report(location, "empty key or embedded NUL");
            if (translations_.contains(key)) return Report(location, std::format("duplicate key {} (first value kept)", key));
            translations_.emplace(std::move(key), std::move(value));
            return true;
        }

        bool LoadTXT(const std::string& text, std::string_view path) {
            constexpr std::size_t minimumKeyLength = 2;  // '$' and at least one character.
            std::istringstream stream(text);
            std::string line;
            std::size_t lineNumber = 0;
            bool success = true;
            while (std::getline(stream, line)) {
                ++lineNumber;
                if (line.empty() || line.front() != '$') continue;
                if (line.back() == '\r') line.pop_back();
                const auto separator = line.rfind('\t');
                const auto location = std::format("{}:{}", path, lineNumber);
                if (separator == std::string::npos || separator < minimumKeyLength) {
                    Report(location, "expected $key followed by a tab and translation");
                    success = false;
                    continue;
                }
                if (!Insert(line.substr(0, separator), UnescapeTXT(std::string_view(line).substr(separator + 1)), location)) success = false;
            }
            return success;
        }

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

        Table defaults_;
        Table translations_;
        ErrorHandler onError_;
    };
}
