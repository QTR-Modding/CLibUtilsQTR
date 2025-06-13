// Author: Quantumyilmaz
// Year: 2025

#include "Tasker.h"

void Tasker::Start(const size_t num_threads) {
    std::lock_guard lock(mutex_);
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&Tasker::WorkerLoop, this);
    }
}


void Tasker::Stop() {
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



void Tasker::WorkerLoop() {
	// At the top of WorkerLoop()
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
            } else {
                // Idle timeout reached: just exit without calling Stop()
                running_.store(false, std::memory_order_release);
                return;
            }
        }

        // 3) We have at least one task. Check the earliest one's scheduled_time
        auto& next = task_queue_.top();

        // If it's time to run the top task now (or in the past):
        if (auto now = std::chrono::steady_clock::now(); next.scheduled_time <= now) {
            Task task = next;
            task_queue_.pop();  // remove it from the queue
            lock.unlock();      // unlock before actually running the task
            try {
                if (task.func) {
                    task.func();
                }
            }
            catch (const std::exception& e) {
				logger::error("Tasker: Exception in task execution: {}", e.what());
			}
            // After running, go back to the top of the loop to check again
            continue;
        }
        
        // 4) Otherwise, do a *timed* wait until the top task's scheduled time
        //    or until we're notified that something changed (like new earlier tasks).
        cv_.wait_until(lock, next.scheduled_time, [this] {
            const auto now = std::chrono::steady_clock::now();  // refresh!
            return !running_.load(std::memory_order_acquire)
                || (!task_queue_.empty() &&
                    task_queue_.top().scheduled_time <= now);
        });

        // Loop around and re-check conditions in case something changed
    }
}
