#pragma once
#include "Authenticode.hpp"

#include <cstdio>
#include <cwchar>

namespace clib_utilsQTR::Signing {
inline int VerifyFileCommandLine(int argc, wchar_t** argv, const SigningKeyHash* defaultKey = nullptr) {
    if ((argc != 2 && argc != 3) || (argc == 2 && !defaultKey)) return 2;
    if (argc == 3 && std::wcslen(argv[2]) != 64) return 2;
    const auto hex = [](wchar_t value) -> int {
        if (value >= L'0' && value <= L'9') return value - L'0';
        if (value >= L'a' && value <= L'f') return value - L'a' + 10;
        if (value >= L'A' && value <= L'F') return value - L'A' + 10;
        return -1;
    };
    auto key = defaultKey ? *defaultKey : SigningKeyHash{};
    for (std::size_t index = 0; argc == 3 && index < key.size(); ++index) {
        const auto high = hex(argv[2][index * 2]);
        const auto low = hex(argv[2][index * 2 + 1]);
        if (high < 0 || low < 0) return 2;
        key[index] = static_cast<BYTE>((high << 4) | low);
    }
    SignedFile file;
    if (!file.Open(argv[1])) {
        std::puts("Cannot read candidate.");
        return 3;
    }
    if (!VerifySignature(file, key)) {
        std::puts("Signature rejected.");
        return 4;
    }
    std::puts("Verified signing key and file digest.");
    return 0;
}

}
