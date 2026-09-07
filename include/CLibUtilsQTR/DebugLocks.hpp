#pragma once

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <type_traits>

namespace clib_utilsQTR {
    namespace detail {
        enum class DebugLockMode { kNone, kShared, kUnique };

        template <class Tag>
        inline thread_local DebugLockMode debug_lock_mode = DebugLockMode::kNone;
    }

    // Use one distinct tag per mutex, shared by its shared and unique guards.
    template <class Tag>
    [[nodiscard]] bool DebugLockHeld() noexcept {
        return detail::debug_lock_mode<Tag> != detail::DebugLockMode::kNone;
    }

    template <class Tag>
    [[noreturn]] void ReportLockViolation(
        const char* reason, const std::source_location& where = std::source_location::current()) noexcept {
        if constexpr (requires { Tag::ReportViolation(Tag::name, reason, where); }) {
            Tag::ReportViolation(Tag::name, reason, where);
        } else {
            std::fprintf(stderr, "[LockAssert] mutex=%s reason=%s at %s:%u (%s)\n",
                         Tag::name, reason, where.file_name(), where.line(), where.function_name());
        }
        std::abort();
    }

    namespace detail {
        template <class Tag, DebugLockMode Mode>
        class DebugLock {
            using Lock = std::conditional_t<Mode == DebugLockMode::kShared,
                                            std::shared_lock<std::shared_mutex>,
                                            std::unique_lock<std::shared_mutex>>;
            Lock lock_;
            std::source_location acquired_at_;

        public:
            explicit DebugLock(std::shared_mutex& mutex,
                               const std::source_location& where = std::source_location::current())
                : lock_(mutex, std::defer_lock), acquired_at_(where) {
                if (DebugLockHeld<Tag>()) {
                    ReportLockViolation<Tag>(
                        debug_lock_mode<Tag> == Mode ? "re-entrant lock acquisition"
                                                     : "shared/unique lock conversion while holding the mutex",
                        where);
                }
                if constexpr (requires { Tag::CheckLockOrder(where); }) {
                    Tag::CheckLockOrder(where);
                }

                lock_.lock();
                debug_lock_mode<Tag> = Mode;
            }

            DebugLock(const DebugLock&) = delete;
            DebugLock& operator=(const DebugLock&) = delete;
            DebugLock(DebugLock&&) = delete;
            DebugLock& operator=(DebugLock&&) = delete;

            void unlock(const std::source_location& where = std::source_location::current()) {
                if (!lock_.owns_lock()) ReportLockViolation<Tag>("unlock without ownership", where);
                if (debug_lock_mode<Tag> != Mode) ReportLockViolation<Tag>("lock ownership state mismatch", where);
                lock_.unlock();
                debug_lock_mode<Tag> = DebugLockMode::kNone;
            }

            ~DebugLock() {
                if (!lock_.owns_lock()) return;
                if (debug_lock_mode<Tag> != Mode) {
                    ReportLockViolation<Tag>("lock ownership state mismatch", acquired_at_);
                }
                debug_lock_mode<Tag> = DebugLockMode::kNone;
            }
        };
    }

    template <class Tag>
    using DebugSharedLock = detail::DebugLock<Tag, detail::DebugLockMode::kShared>;

    template <class Tag>
    using DebugUniqueLock = detail::DebugLock<Tag, detail::DebugLockMode::kUnique>;
}
