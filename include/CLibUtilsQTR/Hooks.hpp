#pragma once

#include <cstdint>
#include <Windows.h>
#include <detours/detours.h>

namespace clib_utilsQTR {
    // Based on https://github.com/RavenKZP/Particle-Wind/blob/bdc66a7415149ee65b1aa87924d0612d6afd7881/include/detour_stl.h#L7
    // Install once, outside another Detours transaction, with matching signatures and calling conventions.
    // Only the current thread is enlisted; prevent other threads from executing the target or destination
    // until the returned original-function address is stored. Keep the destination loaded for the hook's lifetime.
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
