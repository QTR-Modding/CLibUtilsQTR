#pragma once

namespace StringHelpers {
    template <typename T>
    std::string join(const T& container, const std::string_view& delimiter) {
        std::ostringstream oss;
        auto iter = container.begin();

        if (iter != container.end()) {
            oss << *iter;
            ++iter;
        }

        for (; iter != container.end(); ++iter) {
            oss << delimiter << *iter;
        }

        return oss.str();
    }

    inline std::string trim(const std::string& str) {
        // Find the first non-whitespace character from the beginning
        const size_t start = str.find_first_not_of(" \t\n\r");

        // If the string is all whitespace, return an empty string
        if (start == std::string::npos) return "";

        // Find the last non-whitespace character from the end
        const size_t end = str.find_last_not_of(" \t\n\r");

        // Return the substring containing the trimmed characters
        return str.substr(start, end - start + 1);
    }

    inline std::string toLowercase(const std::string& str) {
        std::string result = str;
        std::ranges::transform(result, result.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    inline std::string replaceLineBreaksWithSpace(const std::string& input) {
        std::string result = input;
        std::ranges::replace(result, '\n', ' ');
        return result;
    }

    inline bool includesString(const std::string& input, const std::vector<std::string>& strings) {
        const std::string lowerInput = toLowercase(input);

        for (const auto& str : strings) {
            std::string lowerStr = str;
            std::ranges::transform(lowerStr, lowerStr.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lowerInput.find(lowerStr) != std::string::npos) {
                return true; // The input string includes one of the strings
            }
        }
        return false; // None of the strings in 'strings' were found in the input string
    }

    inline bool includesWord(const std::string& input, const std::vector<std::string>& strings) {
        std::string lowerInput = toLowercase(input);
        lowerInput = replaceLineBreaksWithSpace(lowerInput);
        lowerInput = trim(lowerInput);
        lowerInput = " " + lowerInput + " "; // Add spaces to the beginning and end of the string

        for (const auto& str : strings) {
            std::string lowerStr = str;
            lowerStr = trim(lowerStr);
            lowerStr = " " + lowerStr + " "; // Add spaces to the beginning and end of the string
            std::ranges::transform(lowerStr, lowerStr.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            // logger::trace("lowerInput: {} lowerStr: {}", lowerInput, lowerStr);

            if (lowerInput.find(lowerStr) != std::string::npos) {
                return true; // The input string includes one of the strings
            }
        }
        return false; // None of the strings in 'strings' were found in the input string
    };
} // namespace StringHelpers