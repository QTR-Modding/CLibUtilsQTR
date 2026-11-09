#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace clib_utilsQTR {
    class ThreadPool {
    public:
        explicit ThreadPool(std::size_t count = std::thread::hardware_concurrency());
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        template <class F, class... Args>
        auto enqueue(F&& function, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    private:
        void Run(std::stop_token stop);

        std::mutex mutex;
        std::condition_variable_any ready;
        std::queue<std::move_only_function<void()>> tasks;
        bool stopping{};
        // Workers must join before the queue and synchronization objects are destroyed.
        std::vector<std::jthread> workers;
    };

    inline ThreadPool::ThreadPool(std::size_t count) {
        if (count == 0) {
            count = 1;
        }
        workers.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            workers.emplace_back([this](std::stop_token stop) { Run(stop); });
        }
    }

    inline ThreadPool::~ThreadPool() {
        {
            std::lock_guard lock(mutex);
            stopping = true;
        }
        ready.notify_all();
    }

    inline void ThreadPool::Run(std::stop_token stop) {
        for (;;) {
            std::unique_lock lock(mutex);
            ready.wait(lock, stop, [this] { return stopping || !tasks.empty(); });
            if (tasks.empty()) {
                return;
            }
            auto task = std::move(tasks.front());
            tasks.pop();
            lock.unlock();
            task();
        }
    }

    template <class F, class... Args>
    auto ThreadPool::enqueue(F&& function, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
        using Result = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
        std::packaged_task<Result()> task(
            [call = std::bind_front(std::forward<F>(function), std::forward<Args>(args)...)]() mutable -> Result {
                return std::move(call)();
            });
        auto result = task.get_future();
        {
            std::lock_guard lock(mutex);
            if (stopping) {
                throw std::runtime_error("ThreadPool is stopping.");
            }
            tasks.emplace([task = std::move(task)]() mutable { task(); });
        }
        ready.notify_one();
        return result;
    }
}
