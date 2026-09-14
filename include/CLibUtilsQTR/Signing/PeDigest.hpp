#pragma once

// Copyright (c) 2026 Quantumyilmaz. MIT License.

#include "PeImage.hpp"
#include <wincrypt.h>
#include <algorithm>
#include <array>
#include <vector>

#if defined(_MSC_VER)
#pragma comment(lib, "advapi32.lib")
#endif

namespace clib_utilsQTR::Signing {

struct SignatureDiagnostic {
    const wchar_t* stage{L"none"};
    DWORD error{};
};

inline bool SignatureFailure(SignatureDiagnostic* diagnostic, const wchar_t* stage, DWORD error = 0) {
    if (diagnostic) *diagnostic = {stage, error};
    return false;
}

// Authenticode excludes the checksum, certificate directory entry, and certificate
// table. Hash the remaining headers, sorted sections, then trailing data.
// https://learn.microsoft.com/windows/win32/debug/pe-format#authenticode-pe-image-hash
inline bool ComputePeDigest(std::span<const std::byte> bytes, std::array<BYTE, 32>& digest,
    SignatureDiagnostic* diagnostic = nullptr) {
    if (diagnostic) *diagnostic = {};
    const PeImage image(bytes);
    const auto* nt = image.Headers();
    if (!nt) return SignatureFailure(diagnostic, L"PE headers");
    const auto ntOffset = static_cast<std::size_t>(reinterpret_cast<const std::byte*>(nt) - bytes.data());
    const auto checksum = ntOffset + offsetof(IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);
    const auto directory = ntOffset + offsetof(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory) +
        IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY);
    const auto& certificate = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
    const auto headers = nt->OptionalHeader.SizeOfHeaders;
    const std::size_t end = certificate.VirtualAddress;
    if (headers < ntOffset + sizeof(*nt) + image.Sections().size_bytes() || headers > end ||
        end > bytes.size() || end % 8 != 0 || !certificate.Size || certificate.Size != bytes.size() - end)
        return SignatureFailure(diagnostic, L"PE certificate layout");

    std::vector<IMAGE_SECTION_HEADER> sections(image.Sections().begin(), image.Sections().end());
    std::sort(sections.begin(), sections.end(), [](const auto& a, const auto& b) {
        return a.PointerToRawData < b.PointerToRawData;
    });
    std::size_t sectionEnd = headers;
    for (const auto& section : sections) {
        if (!section.SizeOfRawData) continue;
        // Fail closed on overlapping or gapped sections. These are not the normal
        // linker layout; accepting them risks a different hash/loader interpretation.
        if (section.PointerToRawData != sectionEnd || section.SizeOfRawData > end - sectionEnd)
            return SignatureFailure(diagnostic, L"PE section layout");
        sectionEnd += section.SizeOfRawData;
    }

    struct Hash {
        HCRYPTPROV provider{};
        HCRYPTHASH handle{};
        ~Hash() {
            if (handle) CryptDestroyHash(handle);
            if (provider) CryptReleaseContext(provider, 0);
        }
    } hash;
    if (!CryptAcquireContextW(&hash.provider, nullptr, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return SignatureFailure(diagnostic, L"SHA-256 provider", GetLastError());
    if (!CryptCreateHash(hash.provider, CALG_SHA_256, 0, 0, &hash.handle))
        return SignatureFailure(diagnostic, L"SHA-256 initialization", GetLastError());
    const auto append = [&](std::size_t first, std::size_t last) {
        if (last < first || last > bytes.size() || last - first > MAXDWORD)
            return SignatureFailure(diagnostic, L"PE hash range");
        if (last == first) return true;
        return CryptHashData(hash.handle, reinterpret_cast<const BYTE*>(bytes.data() + first),
            static_cast<DWORD>(last - first), 0) || SignatureFailure(diagnostic, L"SHA-256 data", GetLastError());
    };
    if (!append(0, checksum) || !append(checksum + sizeof(DWORD), directory) ||
        !append(directory + sizeof(IMAGE_DATA_DIRECTORY), headers)) return false;
    for (const auto& section : sections) {
        if (section.SizeOfRawData && !append(section.PointerToRawData,
                static_cast<std::size_t>(section.PointerToRawData) + section.SizeOfRawData)) return false;
    }
    if (!append(sectionEnd, end)) return false;
    DWORD size = static_cast<DWORD>(digest.size());
    if (!CryptGetHashParam(hash.handle, HP_HASHVAL, digest.data(), &size, 0))
        return SignatureFailure(diagnostic, L"SHA-256 result", GetLastError());
    return size == digest.size() || SignatureFailure(diagnostic, L"SHA-256 size");
}

}
