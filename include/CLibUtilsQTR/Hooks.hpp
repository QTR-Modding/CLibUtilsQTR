#pragma once

#include <cstdint>
#include <Windows.h>
#include <detours/detours.h>

namespace clib_utilsQTR {
    // Install while no other thread can execute the target or destination.
    // Returns the original-function trampoline, or 0 if installation fails.
    template <class Func>
    [[nodiscard]] std::uintptr_t write_prologue_hook(std::uintptr_t a_src, Func* a_dest) {
        const auto status = DetourTransactionBegin();
        if (status == ERROR_INVALID_OPERATION) {
            return 0;
        }

        auto target = reinterpret_cast<PVOID>(a_src);
        if (status != NO_ERROR || DetourUpdateThread(GetCurrentThread()) != NO_ERROR ||
            DetourAttach(&target, reinterpret_cast<PVOID>(a_dest)) != NO_ERROR) {
            DetourTransactionAbort();
            return 0;
        }

        if (DetourTransactionCommit() != NO_ERROR) {
            return 0;
        }
        return reinterpret_cast<std::uintptr_t>(target);
    }
} // namespace clib_utilsQTR
