#pragma once

#include <array>
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
            return LoadFile(path, ".txt", [this](const auto& text, const auto& location) {
                return LoadTXT(text, location);
            });
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

    protected:
        template <class Reader>
        bool LoadFile(const std::filesystem::path& path, std::string_view extension, Reader read) {
            translations_.clear();
            if (StringHelpers::toLowercase(path.extension().string()) != extension)
                return Report(path.string(), "unsupported translation file extension");
            std::ifstream file(path, std::ios::binary);
            if (!file) return Report(path.string(), "cannot open translation file");
            const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            if (file.bad()) return Report(path.string(), "cannot read translation file");
            std::string text;
            if (!Decode(bytes, text)) return Report(path.string(), "invalid text encoding (expected UTF-8 or BOM-marked UTF-16LE)");
            return read(text, path.string());
        }

        bool Report(std::string_view location, std::string_view message) const {
            if (onError_) onError_(std::format("{}: {}", location, message));
            return false;
        }

        bool Insert(std::string key, std::string value, std::string_view location) {
            if (key.empty() || key.find('\0') != std::string::npos || value.find('\0') != std::string::npos)
                return Report(location, "empty key or embedded NUL");
            if (translations_.contains(key)) return Report(location, std::format("duplicate key {} (first value kept)", key));
            translations_.emplace(std::move(key), std::move(value));
            return true;
        }

    private:
        [[nodiscard]] std::string_view GetDefault(std::string_view key) const {
            const auto it = defaults_.find(key);
            return it != defaults_.end() ? std::string_view(it->second) : key;
        }

        static constexpr unsigned asciiMax = 0x7F;
        static constexpr unsigned twoByteMax = 0x7FF;
        static constexpr unsigned bmpMax = 0xFFFF;
        static constexpr unsigned unicodeMax = 0x10FFFF;
        static constexpr unsigned highSurrogateFirst = 0xD800;
        static constexpr unsigned lowSurrogateFirst = 0xDC00;
        static constexpr unsigned surrogateLast = 0xDFFF;
        static constexpr unsigned supplementaryFirst = 0x10000;
        static constexpr unsigned surrogateBits = 10;
        static constexpr unsigned byteBits = 8;
        static constexpr unsigned continuationBits = 6;
        static constexpr unsigned continuationTag = 0x80;
        static constexpr unsigned continuationMask = 0x3F;
        static constexpr unsigned byteMask = 0xFF;
        static constexpr std::size_t twoBytes = 2, threeBytes = 3, fourBytes = 4;

        static void AppendUTF8(unsigned codepoint, std::string& output) {
            if (codepoint <= asciiMax) {
                output += static_cast<char>(codepoint);
                return;
            }
            const auto width = codepoint <= twoByteMax ? twoBytes : codepoint <= bmpMax ? threeBytes : fourBytes;
            std::array<char, fourBytes> bytes{};
            for (auto i = width - 1; i > 0; --i) {
                bytes[i] = static_cast<char>(continuationTag | (codepoint & continuationMask));
                codepoint >>= continuationBits;
            }
            bytes[0] = static_cast<char>((byteMask << (byteBits - width)) | codepoint);
            output.append(bytes.data(), width);
        }

        static bool ValidateUTF8(std::string_view text) {
            constexpr unsigned firstTwoByteLead = 0xC2, firstThreeByteLead = 0xE0, firstFourByteLead = 0xF0;
            constexpr unsigned lastFourByteLead = 0xF4, leadMask = 0xC0;
            constexpr std::array<unsigned, fourBytes + 1> minimum{0, 0, asciiMax + 1, twoByteMax + 1, bmpMax + 1};
            for (std::size_t i = 0; i < text.size();) {
                const auto lead = static_cast<unsigned char>(text[i++]);
                if (lead == 0) return false;
                if (lead <= asciiMax) continue;
                if (lead < firstTwoByteLead || lead > lastFourByteLead) return false;
                const auto width = lead < firstThreeByteLead ? twoBytes : lead < firstFourByteLead ? threeBytes : fourBytes;
                if (text.size() - i < width - 1) return false;
                unsigned codepoint = lead & (asciiMax >> width);
                for (std::size_t remaining = width - 1; remaining > 0; --remaining) {
                    const auto tail = static_cast<unsigned char>(text[i++]);
                    if ((tail & leadMask) != continuationTag) return false;
                    codepoint = (codepoint << continuationBits) | (tail & continuationMask);
                }
                if (codepoint < minimum[width] || codepoint > unicodeMax ||
                    (codepoint >= highSurrogateFirst && codepoint <= surrogateLast)) return false;
            }
            return true;
        }

        static bool Decode(const std::string& bytes, std::string& output) {
            constexpr std::string_view utf16BOM = "\xFF\xFE";
            constexpr std::string_view utf8BOM = "\xEF\xBB\xBF";
            if (bytes.starts_with(utf16BOM)) {
                constexpr std::size_t codeUnitBytes = sizeof(char16_t);
                if (bytes.size() % codeUnitBytes != 0) return false;
                const auto readUnit = [&bytes](std::size_t i) {
                    return static_cast<unsigned>(static_cast<unsigned char>(bytes[i])) |
                           (static_cast<unsigned char>(bytes[i + 1]) << byteBits);
                };
                for (auto i = utf16BOM.size(); i < bytes.size(); i += codeUnitBytes) {
                    auto codepoint = readUnit(i);
                    if (codepoint == 0) return false;
                    if (codepoint >= highSurrogateFirst && codepoint < lowSurrogateFirst) {
                        i += codeUnitBytes;
                        if (i == bytes.size()) return false;
                        const auto low = readUnit(i);
                        if (low < lowSurrogateFirst || low > surrogateLast) return false;
                        codepoint = supplementaryFirst + ((codepoint - highSurrogateFirst) << surrogateBits) +
                                    low - lowSurrogateFirst;
                    } else if (codepoint >= lowSurrogateFirst && codepoint <= surrogateLast) {
                        return false;
                    }
                    AppendUTF8(codepoint, output);
                }
                return true;
            }
            auto text = std::string_view(bytes);
            if (text.starts_with(utf8BOM)) text.remove_prefix(utf8BOM.size());
            if (!ValidateUTF8(text)) return false;
            output = text;
            return true;
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

        Table defaults_;
        Table translations_;
        ErrorHandler onError_;
    };
}
