#pragma once

#include "VerifiedProvider.hpp"
#include <atomic>
#include <map>
#include <utility>

namespace clib_utilsQTR::Signing {

// Own this object for as long as any returned function pointer is used.
// Missing modules can be retried. Verification failures return an error, never
// call unverified exports, and do not display dialogs or terminate the process.
class ProviderBinding {
public:
    ProviderBinding(std::wstring moduleName, SigningKeyHash key) :
        moduleName_(std::move(moduleName)), key_(key) {}

    VerifiedProvider* Get(BindingError& error) {
        std::lock_guard lock(mutex_);
        return GetLocked(error);
    }

    FARPROC Resolve(std::string_view name, BindingError& error) {
        std::lock_guard lock(mutex_);
        auto* provider = GetLocked(error);
        if (!provider) return nullptr;
        if (const auto found = exports_.find(name); found != exports_.end()) return found->second;
        const auto function = provider->Resolve(name, error);
        if (error != BindingError::None) return nullptr;
        exports_.emplace(name, function);
        return function;
    }

private:
    VerifiedProvider* GetLocked(BindingError& error) {
        error = BindingError::None;
        if (provider_) return provider_.get();
        auto module = FindProvider(moduleName_, error);
        if (error != BindingError::None) return nullptr;
        auto candidate = std::make_unique<VerifiedProvider>();
        error = candidate->Bind(module, key_);
        if (error != BindingError::None) return nullptr;
        // Export caches may be used during client teardown. Never unload code
        // whose function pointers have been handed to the caller.
        HMODULE pinned{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(module), &pinned) || pinned != module) {
            error = BindingError::Inspection;
            return nullptr;
        }
        provider_ = std::move(candidate);
        return provider_.get();
    }

    const std::wstring moduleName_;
    const SigningKeyHash key_;
    std::mutex mutex_;
    std::unique_ptr<VerifiedProvider> provider_;
    std::map<std::string, FARPROC, std::less<>> exports_;
};

// Resolve is a caller-owned policy function: it must handle verification errors
// before returning. Missing optional exports remain retryable until a host loads.
template<class T, auto Resolve>
class OptionalFunction {
public:
    explicit OptionalFunction(const char* name) : name_(name) {}
    explicit operator bool() const { return Get() != nullptr; }
    operator T() const { return Get(); }
    template<class... Args>
    decltype(auto) operator()(Args&&... args) const { return Get()(std::forward<Args>(args)...); }

private:
    T Get() const {
        auto value = value_.load(std::memory_order_acquire);
        if (!value) {
            value = reinterpret_cast<T>(Resolve(name_));
            if (value) value_.store(value, std::memory_order_release);
        }
        return value;
    }
    const char* name_;
    mutable std::atomic<T> value_{};
};

}
