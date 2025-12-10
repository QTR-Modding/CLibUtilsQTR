// Author: Quantumyilmaz
// Year: 2025
#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <REX/REX/Singleton.h>

namespace clib_utilsQTR {
    struct Task {
        std::chrono::steady_clock::time_point scheduled_time;
        std::function<void()> func;

        Task() = default;

        Task(std::function<void()> f, const std::chrono::steady_clock::time_point time)
            : scheduled_time(time), func(std::move(f)) {
        }

        bool operator>(const Task& other) const {
            return scheduled_time > other.scheduled_time;
        }
    };

    class Tasker final : public REX::Singleton<Tasker> {
    public:
        void Start(size_t num_threads = std::thread::hardware_concurrency()) {
            std::lock_guard lock(mutex_);
            if (running_.load(std::memory_order_acquire)) {
                return;
            }

            running_.store(true, std::memory_order_release);
            for (size_t i = 0; i < num_threads; ++i) {
                workers_.emplace_back(&Tasker::WorkerLoop, this);
            }
        }

        void Stop() {
            {
                std::lock_guard lock(mutex_);
                if (!running_.load(std::memory_order_acquire)) {
                    if (workers_.empty()) {
                        return;
                    }
                }
                running_.store(false, std::memory_order_release);
            }

            cv_.notify_all();

            const auto this_id = std::this_thread::get_id();
            for (auto& t : workers_) {
                if (t.joinable() && t.get_id() != this_id) {
                    t.join();
                }
            }

            {
                std::lock_guard lock(mutex_);
                workers_.clear();
            }
        }

        bool IsRunning() const {
            std::lock_guard lock(mutex_);
            return running_.load(std::memory_order_acquire);
        }

        bool HasTask() const {
            std::lock_guard lock(mutex_);
            return !task_queue_.empty();
        }


        template <typename Func, typename... Args>
        void PushTask(Func&& f, const int delay_ms, Args&&... args) {
            if (!IsRunning()) {
                Start();
            }
            {
                auto bound_func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
                const auto scheduled_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
                std::lock_guard lock(mutex_);
                task_queue_.emplace(std::move(bound_func), scheduled_time);
            }
            cv_.notify_one();
        }


        /**
         * @brief Schedules a task to be executed after a condition has been continuously true for a specified duration.
         *
         * This function repeatedly checks a user-provided condition at regular intervals. If the condition remains true 
         * continuously for `duration_ms` milliseconds, the provided function `f` is executed. If at any point the condition
         * returns false, the task chain is stopped.
         *
         * @tparam Condition A callable type that takes no parameters and returns a `bool`.  
         * @tparam Func A callable type representing the task to be executed.
         *
         * @param cond The condition to be checked periodically. Must return a `bool`.
         * @param duration_ms The duration in milliseconds for which the condition must remain continuously true before `f` is invoked.
         * @param f The task to execute after the sustained condition is met.
         * @param poll_interval_ms The interval in milliseconds between successive condition checks. Defaults to 50ms.
         *
         * @details 
         * The function leverages `Tasker::PushTask` to schedule repeated checks on the condition. If the condition holds
         * for the entirety of the `duration_ms` period, the task `f` will be called exactly once. If the condition becomes 
         * false at any point before the duration elapses, no further checks are performed, and `f` will not be called.
         *
         * This mechanism is useful for scenarios where an action should only occur if a condition is stable over time, 
         * such as sustained user input, stable sensor readings, or debounce logic.
         *
         * @note Requires `Condition` to be invocable and to return a `bool`. `Func` must be invocable with no parameters.
         *
         * @example
         * @code
         * PushSustainedTask(
         *     []() { return IsButtonPressed(); }, // Condition
         *     1000,                               // 1000 ms duration
         *     []() { DoAction(); }                // Task to execute
         * );
         * @endcode
         */
        template <typename Condition, typename Func>
            requires std::invocable<Condition> && std::is_same_v<std::invoke_result_t<Condition>, bool>
        static void PushSustainedTask(Condition cond, int duration_ms, Func f, int poll_interval_ms = 50) {
            auto start = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
            auto checker = std::make_shared<std::function<void()>>();
            *checker = [cond, f, duration_ms, poll_interval_ms, start, checker]() mutable {
                if (!cond()) {
                    //logger::info("Condition not met, stopping sustained task.");
                    return;
                }
                const auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - *start).count() >= duration_ms) {
                    f();
                } else {
                    //logger::info("Condition met, but duration not reached yet. Checking again in {} ms.", poll_interval_ms);
                    GetSingleton()->PushTask([checker] { (*checker)(); }, poll_interval_ms);
                }
            };
            GetSingleton()->PushTask([checker] { (*checker)(); }, 0);
        }

    private:
        std::priority_queue<Task, std::vector<Task>, std::greater<>> task_queue_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::atomic<bool> running_{false};
        std::vector<std::thread> workers_;

        void WorkerLoop() {
            constexpr auto idle_timeout = std::chrono::seconds(5);

            for (;;) {
                std::unique_lock lock(mutex_);

                // 1) If we have been told to stop and there is nothing left to do, exit
                if (!running_.load(std::memory_order_acquire) && task_queue_.empty()) {
                    return;
                }

                if (task_queue_.empty()) {
                    // Wait with timeout to detect idleness
                    if (cv_.wait_for(lock, idle_timeout, [this] {
                        return !running_.load(std::memory_order_acquire) || !task_queue_.empty();
                    })) {
                        // Woke up because we have a task or we're stopping
                        continue;
                    }
                    // Idle timeout reached: just exit without calling Stop()
                    running_.store(false, std::memory_order_release);
                    return;
                }

                // 3) We have at least one task. Check the earliest one's scheduled_time
                auto& next = task_queue_.top();

                // If it's time to run the top task now (or in the past):
                if (auto now = std::chrono::steady_clock::now(); next.scheduled_time <= now) {
                    Task task = next;
                    task_queue_.pop(); // remove it from the queue
                    lock.unlock(); // unlock before actually running the task
                    try {
                        if (task.func) {
                            task.func();
                        }
                    } catch ([[maybe_unused]] const std::exception& e) {
                        //logger::error("Tasker: Exception in task execution: {}", e.what());
                    }
                    // After running, go back to the top of the loop to check again
                    continue;
                }

                // 4) Otherwise, do a *timed* wait until the top task's scheduled time
                //    or until we're notified that something changed (like new earlier tasks).
                cv_.wait_until(lock, next.scheduled_time, [this] {
                    const auto now = std::chrono::steady_clock::now(); // refresh!
                    return !running_.load(std::memory_order_acquire)
                           || (!task_queue_.empty() &&
                               task_queue_.top().scheduled_time <= now);
                });

                // Loop around and re-check conditions in case something changed
            }
        }
    };
}
