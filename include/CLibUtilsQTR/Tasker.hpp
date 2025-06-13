// Author: Quantumyilmaz
// Year: 2025

#pragma once

struct Task {
    std::chrono::steady_clock::time_point scheduled_time;
    std::function<void()> func;

	Task() = default;
    Task(std::function<void()> f, const std::chrono::steady_clock::time_point time)
        : scheduled_time(time), func(std::move(f)) {}

    bool operator>(const Task& other) const {
        return scheduled_time > other.scheduled_time;
    }
};

class Tasker final : public clib_util::singleton::ISingleton<Tasker> {
public:

    void Start(size_t num_threads = std::thread::hardware_concurrency());

    void Stop();

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
                Tasker::GetSingleton()->PushTask([checker]{ (*checker)(); }, poll_interval_ms);
            }
        };
        Tasker::GetSingleton()->PushTask([checker]{ (*checker)(); }, 0);
    }

private:

    std::priority_queue<Task, std::vector<Task>, std::greater<>> task_queue_;
	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> running_{ false };
	std::vector<std::thread> workers_;

	void WorkerLoop();
};
