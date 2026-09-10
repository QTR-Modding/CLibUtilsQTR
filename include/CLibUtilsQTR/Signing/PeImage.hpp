#pragma once

// Copyright (c) 2026 Quantumyilmaz. MIT License.

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

namespace clib_utilsQTR::Signing {

// A file-layout PE view. No pointer from an unverified export is ever called.
class PeImage {
public:
    explicit PeImage(std::span<const std::byte> bytes) : bytes_(bytes) {}

    template <class T>
    const T* At(std::size_t offset, std::size_t count = 1) const {
        if (offset > bytes_.size() || count > (bytes_.size() - offset) / sizeof(T)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(bytes_.data() + offset);
    }

    const IMAGE_NT_HEADERS64* Headers() const {
        const auto* dos = At<IMAGE_DOS_HEADER>(0);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) return nullptr;
        const auto* nt = At<IMAGE_NT_HEADERS64>(static_cast<std::size_t>(dos->e_lfanew));
        if (!nt || nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            nt->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64) ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
            nt->OptionalHeader.NumberOfRvaAndSizes != IMAGE_NUMBEROF_DIRECTORY_ENTRIES ||
            !nt->FileHeader.NumberOfSections || nt->FileHeader.NumberOfSections > 96 ||
            !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL)) return nullptr;
        const auto sectionOffset = static_cast<std::size_t>(dos->e_lfanew) + sizeof(*nt);
        return At<IMAGE_SECTION_HEADER>(sectionOffset, nt->FileHeader.NumberOfSections) ? nt : nullptr;
    }

    std::span<const IMAGE_SECTION_HEADER> Sections() const {
        const auto* nt = Headers();
        if (!nt) return {};
        const auto* sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(nt + 1);
        return {sections, nt->FileHeader.NumberOfSections};
    }

    template <class T>
    const T* Rva(DWORD rva, std::size_t count = 1) const {
        const auto* nt = Headers();
        if (!nt || count > SIZE_MAX / sizeof(T)) return nullptr;
        const auto size = count * sizeof(T);
        if (rva < nt->OptionalHeader.SizeOfHeaders && size <= nt->OptionalHeader.SizeOfHeaders - rva) {
            return At<T>(rva, count);
        }
        for (const auto& section : Sections()) {
            if (rva >= section.VirtualAddress) {
                const auto offset = rva - section.VirtualAddress;
                if (offset <= section.SizeOfRawData && size <= section.SizeOfRawData - offset) {
                    return At<T>(static_cast<std::size_t>(section.PointerToRawData) + offset, count);
                }
            }
        }
        return nullptr;
    }

    bool NameEquals(DWORD rva, std::string_view name) const {
        const auto* text = Rva<char>(rva, name.size() + 1);
        return text && text[name.size()] == '\0' && std::memcmp(text, name.data(), name.size()) == 0;
    }

    bool Executable(DWORD rva) const {
        for (const auto& section : Sections()) {
            if (rva >= section.VirtualAddress && rva - section.VirtualAddress < section.SizeOfRawData) {
                return (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            }
        }
        return false;
    }

private:
    std::span<const std::byte> bytes_;
};

}
