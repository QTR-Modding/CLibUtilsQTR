#include <CLibUtilsQTR/Signing.hpp>
#include "TestKey.hpp"
#include <iostream>

namespace S = clib_utilsQTR::Signing;
int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) return 1;
    const std::wstring_view mode{argv[1]};
    S::BindingError error{};
    S::VerifiedProvider unbound;
    if (unbound.Resolve("ExampleValue", error) || error != S::BindingError::Inspection) return 2;
    S::ProviderBinding binding{L"ExampleProvider.dll", testKey};
    if (mode == L"retry" && (binding.Get(error) || error != S::BindingError::Missing)) return 3;
    const auto module = LoadLibraryW(argv[2]);
    if (!module) return 4;
    if (mode == L"reject") {
        if (binding.Resolve("ExampleValue", error) || error != S::BindingError::Signature) return 5;
        FreeLibrary(module);
        return 0;
    }
    const auto function = GetProcAddress(module, "ExampleValue");
    if (!function) return 6;
    if (mode == L"code") {
        DWORD old{}, ignored{};
        auto* address = reinterpret_cast<BYTE*>(function);
        if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &old)) return 7;
        const BYTE saved = *address;
        *address ^= 1;
        const auto result = binding.Get(error);
        *address = saved;
        VirtualProtect(address, 1, old, &ignored);
        if (result || error != S::BindingError::Image) return 8;
    }
    const auto resolved = binding.Resolve("ExampleValue", error);
    if (error != S::BindingError::None || resolved != function ||
        reinterpret_cast<int(*)()>(resolved)() != 42) return 9;
    if (binding.Resolve("MissingOptional", error) || error != S::BindingError::None) return 10;
    if (mode == L"exports") {
        S::VerifiedProvider provider;
        if (provider.Bind(module, testKey) != S::BindingError::None) return 11;
        auto* base = reinterpret_cast<std::byte*>(module);
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        auto* exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base +
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        auto* slot = reinterpret_cast<DWORD*>(base + exports->AddressOfFunctions);
        DWORD old{}, ignored{};
        if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &old)) return 12;
        const auto saved = *slot;
        *slot = 1;
        const auto changed = provider.Resolve("ExampleValue", error);
        *slot = saved;
        VirtualProtect(slot, sizeof(*slot), old, &ignored);
        if (changed || error != S::BindingError::Export) return 13;
    }
    FreeLibrary(module);
    if (reinterpret_cast<int(*)()>(binding.Resolve("ExampleValue", error))() != 42) return 14;
    std::cout << "Verified provider binding passed\n";
}
