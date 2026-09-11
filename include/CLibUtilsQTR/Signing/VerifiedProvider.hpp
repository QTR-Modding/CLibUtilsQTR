#pragma once

// Copyright (c) 2026 Quantumyilmaz. MIT License.

#include "Authenticode.hpp"

#include <psapi.h>

#include <algorithm>
#include <memory>
#include <mutex>

namespace clib_utilsQTR::Signing {

enum class BindingError { None, Missing, Inspection, Signature, Image, Export };

// A binding owns a reference to the loaded image and a read-locked signed file.
// Existing function-pointer caches therefore cannot outlive either identity.
class VerifiedProvider {
public:
    VerifiedProvider() = default;
    VerifiedProvider(const VerifiedProvider&) = delete;
    VerifiedProvider& operator=(const VerifiedProvider&) = delete;
    ~VerifiedProvider() { if (module_) FreeLibrary(module_); }

    BindingError Bind(HMODULE candidate, const SigningKeyHash& key) {
        if (attempted_) return BindingError::Inspection;
        attempted_ = true;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>(candidate), &module_) || module_ != candidate) return BindingError::Inspection;
        std::array<wchar_t, 32768> path{};
        const auto length = GetModuleFileNameW(module_, path.data(), static_cast<DWORD>(path.size()));
        if (!length || length >= path.size() || !file_.Open(path.data())) return BindingError::Inspection;
        if (!VerifySignature(file_, key)) return BindingError::Signature;
        const PeImage image(file_.bytes);
        const auto* nt = image.Headers();
        const auto* dos = image.At<IMAGE_DOS_HEADER>(0);
        if (!nt || !dos) return BindingError::Image;
        size_ = nt->OptionalHeader.SizeOfImage;
        const auto headerSize = static_cast<std::size_t>(dos->e_lfanew) + sizeof(*nt) +
            image.Sections().size_bytes();
        // Windows can replace OptionalHeader.ImageBase when applying ASLR.
        const auto baseOffset = static_cast<std::size_t>(dos->e_lfanew) +
            offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + offsetof(IMAGE_OPTIONAL_HEADER64, ImageBase);
        const auto loadedBase = reinterpret_cast<ULONGLONG>(module_);
        const auto afterBase = baseOffset + sizeof(loadedBase);
        if (!Matches(0, file_.bytes.data(), baseOffset) ||
            (!Matches(baseOffset, &nt->OptionalHeader.ImageBase, sizeof(loadedBase)) &&
             !Matches(baseOffset, &loadedBase, sizeof(loadedBase))) ||
            !Matches(afterBase, file_.bytes.data() + afterBase, headerSize - afterBase) ||
            !MatchesExecutableImage(image)) return BindingError::Image;
        const auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto* exports = image.Rva<IMAGE_EXPORT_DIRECTORY>(directory.VirtualAddress);
        if (!exports || directory.Size < sizeof(*exports) || !exports->NumberOfFunctions ||
            exports->NumberOfFunctions > 65536 || exports->NumberOfNames > 65536 ||
            !image.Rva<DWORD>(exports->AddressOfFunctions, exports->NumberOfFunctions) ||
            !image.Rva<DWORD>(exports->AddressOfNames, exports->NumberOfNames) ||
            !image.Rva<WORD>(exports->AddressOfNameOrdinals, exports->NumberOfNames) ||
            !Matches(directory.VirtualAddress, exports, sizeof(*exports))) return BindingError::Image;
        verified_ = true;
        return BindingError::None;
    }

    FARPROC Resolve(std::string_view name, BindingError& error) const {
        error = BindingError::None;
        if (!verified_) { error = BindingError::Inspection; return nullptr; }
        const PeImage image(file_.bytes);
        const auto directory = image.Headers()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto* exports = image.Rva<IMAGE_EXPORT_DIRECTORY>(directory.VirtualAddress);
        const auto* names = image.Rva<DWORD>(exports->AddressOfNames, exports->NumberOfNames);
        const auto* ordinals = image.Rva<WORD>(exports->AddressOfNameOrdinals, exports->NumberOfNames);
        const auto* functions = image.Rva<DWORD>(exports->AddressOfFunctions, exports->NumberOfFunctions);
        for (DWORD index = 0; index < exports->NumberOfNames; ++index) {
            if (!image.NameEquals(names[index], name)) continue;
            const auto ordinal = ordinals[index];
            if (ordinal >= exports->NumberOfFunctions) { error = BindingError::Export; return nullptr; }
            const auto rva = functions[ordinal];
            const bool forwarded = rva >= directory.VirtualAddress && rva - directory.VirtualAddress < directory.Size;
            if (!rva || rva >= size_ || forwarded || !image.Executable(rva) ||
                !Matches(directory.VirtualAddress, exports, sizeof(*exports)) ||
                !Matches(exports->AddressOfNames + index * sizeof(DWORD), names + index, sizeof(DWORD)) ||
                !Matches(exports->AddressOfNameOrdinals + index * sizeof(WORD), ordinals + index, sizeof(WORD)) ||
                !Matches(names[index], name.data(), name.size()) ||
                !Matches(exports->AddressOfFunctions + ordinal * sizeof(DWORD), functions + ordinal, sizeof(DWORD))) {
                error = BindingError::Export;
                return nullptr;
            }
            const auto* address = reinterpret_cast<const std::byte*>(module_) + rva;
            MEMORY_BASIC_INFORMATION memory{};
            if (!VirtualQuery(address, &memory, sizeof(memory)) || memory.AllocationBase != module_ ||
                memory.Type != MEM_IMAGE || memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD) ||
                !(memory.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
                error = BindingError::Export;
                return nullptr;
            }
            return reinterpret_cast<FARPROC>(const_cast<std::byte*>(address));
        }
        // Optional exports remain optional on an authenticated host.
        return nullptr;
    }

    HMODULE Module() const { return verified_ ? module_ : nullptr; }

private:
    bool ReadRelocations(const PeImage& image, std::vector<DWORD>& relocations) const {
        const auto directory = image.Headers()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (!directory.VirtualAddress && !directory.Size) return true;
        if (!directory.VirtualAddress || directory.Size < sizeof(IMAGE_BASE_RELOCATION)) return false;
        const auto* bytes = image.Rva<std::byte>(directory.VirtualAddress, directory.Size);
        if (!bytes) return false;
        std::size_t offset{};
        while (offset < directory.Size) {
            if (directory.Size - offset < sizeof(IMAGE_BASE_RELOCATION)) return false;
            IMAGE_BASE_RELOCATION block{};
            std::memcpy(&block, bytes + offset, sizeof(block));
            if (block.SizeOfBlock < sizeof(block) || block.SizeOfBlock > directory.Size - offset ||
                (block.SizeOfBlock - sizeof(block)) % sizeof(WORD) != 0 ||
                (block.VirtualAddress & 0xFFF) != 0) return false;
            const auto count = (block.SizeOfBlock - sizeof(block)) / sizeof(WORD);
            for (std::size_t index = 0; index < count; ++index) {
                WORD entry{};
                std::memcpy(&entry, bytes + offset + sizeof(block) + index * sizeof(entry), sizeof(entry));
                const auto type = entry >> 12;
                if (type == IMAGE_REL_BASED_ABSOLUTE) continue;
                if (type != IMAGE_REL_BASED_DIR64) return false;
                const auto target = static_cast<std::uint64_t>(block.VirtualAddress) + (entry & 0xFFF);
                if (target > size_ || sizeof(ULONGLONG) > size_ - target) return false;
                relocations.push_back(static_cast<DWORD>(target));
            }
            offset += block.SizeOfBlock;
        }
        std::sort(relocations.begin(), relocations.end());
        for (std::size_t index = 1; index < relocations.size(); ++index) {
            if (relocations[index] - relocations[index - 1] < sizeof(ULONGLONG)) return false;
        }
        return true;
    }

    // Compare code once against the authenticated file, applying only the
    // loader's documented x64 address relocations. Writable runtime state and
    // import slots are not used as a substitute for verifying executable bytes.
    bool MatchesExecutableImage(const PeImage& image) const {
        std::vector<DWORD> relocations;
        if (!ReadRelocations(image, relocations)) return false;
        const auto delta = reinterpret_cast<std::uintptr_t>(module_) - image.Headers()->OptionalHeader.ImageBase;
        bool foundCode{};
        for (const auto& section : image.Sections()) {
            if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
            const auto extent = (std::max)(section.Misc.VirtualSize, section.SizeOfRawData);
            if (!extent) continue;
            if (extent > 512 * 1024 * 1024 || section.VirtualAddress > size_ ||
                extent > size_ - section.VirtualAddress) return false;
            const auto* raw = image.At<std::byte>(section.PointerToRawData, section.SizeOfRawData);
            if (!raw) return false;
            std::vector<std::byte> expected(extent);
            if (section.SizeOfRawData) std::memcpy(expected.data(), raw, section.SizeOfRawData);
            const auto end = static_cast<std::uint64_t>(section.VirtualAddress) + extent;
            for (const auto target : relocations) {
                if (static_cast<std::uint64_t>(target) + sizeof(ULONGLONG) <= section.VirtualAddress || target >= end) continue;
                if (target < section.VirtualAddress || sizeof(ULONGLONG) > extent - (target - section.VirtualAddress)) return false;
                const auto offset = target - section.VirtualAddress;
                ULONGLONG value{};
                std::memcpy(&value, expected.data() + offset, sizeof(value));
                value += delta;
                std::memcpy(expected.data() + offset, &value, sizeof(value));
            }
            if (!Matches(section.VirtualAddress, expected.data(), expected.size(), true)) return false;
            foundCode = true;
        }
        return foundCode;
    }

    bool Matches(std::size_t rva, const void* expected, std::size_t size, bool executable = false) const {
        if (rva > size_ || size > size_ - rva) return false;
        const auto* address = reinterpret_cast<const std::byte*>(module_) + rva;
        const auto* data = static_cast<const std::byte*>(expected);
        while (size) {
            MEMORY_BASIC_INFORMATION memory{};
            if (!VirtualQuery(address, &memory, sizeof(memory)) || memory.AllocationBase != module_ ||
                memory.Type != MEM_IMAGE || memory.State != MEM_COMMIT ||
                (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
                !(memory.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                    PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) ||
                (executable && !(memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))) return false;
            const auto available = memory.RegionSize - static_cast<std::size_t>(address -
                static_cast<const std::byte*>(memory.BaseAddress));
            const auto chunk = (size < available) ? size : available;
            if (!chunk || std::memcmp(address, data, chunk) != 0) return false;
            size -= chunk;
            address += chunk;
            data += chunk;
        }
        return true;
    }

    HMODULE module_{};
    DWORD size_{};
    bool attempted_{};
    bool verified_{};
    SignedFile file_;
};

inline HMODULE FindProvider(std::wstring_view moduleName, BindingError& error) {
    const std::wstring requestedName{moduleName};
    error = BindingError::None;
    std::vector<HMODULE> modules(256);
    DWORD required{};
    bool complete{};
    for (unsigned retry = 0; retry < 4; ++retry) {
        if (!K32EnumProcessModules(GetCurrentProcess(), modules.data(),
                static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &required)) {
            error = BindingError::Inspection;
            return nullptr;
        }
        if (required <= modules.size() * sizeof(HMODULE)) { complete = true; break; }
        if (required > 65536 * sizeof(HMODULE)) { error = BindingError::Inspection; return nullptr; }
        modules.resize(required / sizeof(HMODULE) + 16);
    }
    if (!complete) { error = BindingError::Inspection; return nullptr; }
    HMODULE found{};
    for (std::size_t index = 0; index < required / sizeof(HMODULE); ++index) {
        std::array<wchar_t, 32768> path{};
        const auto length = GetModuleFileNameW(modules[index], path.data(), static_cast<DWORD>(path.size()));
        if (!length || length >= path.size()) { error = BindingError::Inspection; return nullptr; }
        const wchar_t* basename = std::wcsrchr(path.data(), L'\\');
        basename = basename ? basename + 1 : path.data();
        if (_wcsicmp(basename, requestedName.c_str()) != 0) continue;
        if (found) { error = BindingError::Image; return nullptr; }
        found = modules[index];
    }
    if (!found) error = BindingError::Missing;
    return found;
}

}
