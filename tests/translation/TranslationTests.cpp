#include <CLibUtilsQTR/Translator.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    void Check(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    struct Fixture {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            std::format("qtr-translation-{}", std::chrono::steady_clock::now().time_since_epoch().count());

        Fixture() { Check(std::filesystem::create_directory(directory), "create fixture directory"); }
        ~Fixture() { std::error_code ignored; std::filesystem::remove_all(directory, ignored); }

        std::filesystem::path Write(std::string_view name, std::string_view bytes) const {
            const auto path = directory / name;
            std::ofstream file(path, std::ios::binary);
            file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            Check(static_cast<bool>(file), "write fixture");
            return path;
        }
    };

    std::string UTF16LE(std::u16string_view text) {
        std::string bytes = "\xFF\xFE";
        constexpr unsigned byteBits = 8;
        constexpr unsigned byteMask = 0xFF;
        for (const auto unit : text) {
            bytes += static_cast<char>(unit & byteMask);
            bytes += static_cast<char>(unit >> byteBits);
        }
        return bytes;
    }

    void RunTests() {
        Fixture fixture;
        std::vector<std::string> errors;
        clib_utilsQTR::Translator text({{"$Hello", "Hello {}"}, {"$Empty", "Default"}, {"$Missing", "English"}},
            [&](std::string_view error) { errors.emplace_back(error); });
        Check(text.Format("$Hello", "player") == "Hello player", "built-in format");
        Check(text.Get("$Unknown") == "$Unknown", "unknown key");

        const auto txt = fixture.Write("language.TXT", UTF16LE(
            u"comment\r\n$Hello\tHallo {}\r\n\r\n$Empty\t\r\n$Unicode\t\u00E4\u4E16\U0001F600\r\n"
            u"$Escaped\ta\\nb\\tc\\\\d\\q\\r\r\n$Last\tend"));
        Check(text.Load(txt), "UTF-16LE file");
        Check(text.Format("$Hello", "player") == "Hallo player", "translated format");
        Check(text.Get("$Missing") == "English", "missing translation fallback");
        Check(text.Get("$Empty").empty(), "explicit empty translation");
        Check(text.Get("$Unicode") == "\xC3\xA4\xE4\xB8\x96\xF0\x9F\x98\x80", "Unicode conversion");
        Check(text.Get("$Escaped") == "a\nb\tc\\d\\q\\r", "TXT escapes exactly once");
        Check(text.Get("$Last") == "end", "blank lines do not end parsing");
        Check(errors.empty(), "valid TXT diagnostics");

        Check(text.Load(fixture.Write("utf8.txt", "\xEF\xBB\xBF$Hello\tBonjour {}")), "UTF-8 BOM");
        Check(text.Get("$Last") == "$Last", "reload removes stale keys");
        const std::string longValue(4096, 'x');
        Check(text.Load(fixture.Write("long.txt", "$Long\t" + longValue)), "long line");
        Check(text.Get("$Long") == longValue, "long line is not truncated");

        Check(!text.Load(fixture.Write("partial.txt", "$Hello\tfirst\n$Hello\tsecond\n$Broken\n$Empty\t\n")), "TXT errors");
        Check(text.Get("$Hello") == "first", "first duplicate wins");
        Check(errors.size() == 2 && errors[0].find(":2:") != std::string::npos &&
            errors[1].find(":3:") != std::string::npos, "TXT line diagnostics");
        errors.clear();

        Check(text.Load(fixture.Write("language.json", R"({"$Hello":"Salut {}","$Empty":"","$Escaped":"a\nb\tc\\n","plain":"value"})")), "JSON file");
        Check(text.Format("$Hello", 42) == "Salut 42", "JSON format");
        Check(text.Get("$Escaped") == "a\nb\tc\\n", "JSON escapes exactly once");
        Check(text.Get("plain") == "value" && text.Get("$Empty").empty(), "JSON keys and empty values");
        Check(!text.Load(fixture.Write("partial.json", R"({"$Hello":"first","$Hello":"second","$Missing":false,"ok":"yes","":"bad","nul":"\u0000"})")), "JSON entry errors");
        Check(text.Get("$Hello") == "first" && text.Get("ok") == "yes" && text.Get("$Missing") == "English", "partial JSON fallback");
        Check(errors.size() == 4 && errors[0].find("$Hello") != std::string::npos, "JSON diagnostics");
        errors.clear();

        Check(text.Load(fixture.Write("format.json", R"({"$Hello":"{1}","$Bad":"{"})")), "load bad format");
        Check(text.Format("$Hello", "player") == "Hello player", "format error falls back to English");
        Check(text.Format("$Bad") == "$Bad", "format error without a default");
        Check(errors.size() == 2, "format diagnostics");
        clib_utilsQTR::Translator badDefaults({{"$Bad", "{"}});
        Check(badDefaults.Format("$Bad") == "{", "invalid default format stays readable");

        const std::vector<std::pair<std::string, std::string>> invalid = {
            {"broken.json", "{\"$Hello\":\"changed\","}, {"array.json", "[]"},
            {"nul.json", std::string("{}\0trailing", sizeof("{}\0trailing") - 1)},
            {"utf8.txt", "$Hello\t\xF0\x9F"}, {"overlong.txt", "$Hello\t\xC0\x80"},
            {"odd.txt", UTF16LE(u"$Hello\tx") + 'x'},
            {"surrogate.txt", UTF16LE(std::u16string{u'$', u'X', u'\t', static_cast<char16_t>(0xD800)})},
            {"nul.txt", UTF16LE(std::u16string{u'$', u'X', u'\t', u'\0', u'x'})},
            {"unsupported.ini", "$Hello\tchanged"}
        };
        for (const auto& [name, bytes] : invalid) {
            Check(!text.Load(fixture.Write(name, bytes)), name);
            Check(text.Format("$Hello", "player") == "Hello player", "invalid file clears overrides");
        }
        Check(!text.Load(fixture.directory / "missing.txt"), "missing file");
        Check(text.Get("$Missing") == "English", "missing file fallback");
        Check(text.Load(fixture.Write("empty.txt", "")), "empty TXT uses defaults");
        Check(text.Load(fixture.Write("bom.txt", "\xFF\xFE")), "empty UTF-16 TXT");
        Check(text.Load(fixture.Write("utf16.json", UTF16LE(u"{\"$Hello\":\"Hallo {}\"}"))), "UTF-16 JSON");
    }
}

int main(int argc, char** argv) {
    try {
        RunTests();
        if (argc > 1) {
            clib_utilsQTR::Translator actual({}, [](std::string_view error) { std::cerr << error << '\n'; });
            Check(actual.Load(argv[1]), "existing translation file");
            Check(actual.Get("$SkyPromptTutorialQuit") == "Quit Tutorial", "existing tutorial key");
            Check(actual.Get("$SkyPromptTutorialMenuInfo").find('\n') != std::string_view::npos, "existing escaped text");
        }
        std::cout << "Translation tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
