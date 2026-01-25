#pragma once
#include <functional>

class Ticker {
    std::function<void()> m_OnTick;
    std::chrono::milliseconds m_Interval;
    std::chrono::milliseconds m_RemainingInterval;

    std::thread m_Thread;
    std::atomic<bool> m_Running;
    std::atomic<bool> m_Paused;

    std::mutex m_Mutex;
    std::condition_variable m_Condition;

    void RunLoop() {
        std::unique_lock lock(m_Mutex);

        while (m_Running) {
            if (m_Paused) {
                m_Condition.wait(lock, [this] { return !m_Paused || !m_Running; });
                continue;
            }

            auto start = std::chrono::steady_clock::now();
            m_Condition.wait_for(lock, m_RemainingInterval, [this] { return !m_Running || m_Paused; });

            if (!m_Running) break;

            if (m_Paused) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                m_RemainingInterval = std::max(m_RemainingInterval - elapsed, std::chrono::milliseconds(0));
                continue;
            }

            lock.unlock();
            try {
                m_OnTick();
            } catch (...) {
                Stop();
            }
            lock.lock();

            m_RemainingInterval = m_Interval;
        }
    }

public:
    void Stop() {
        {
            std::lock_guard lock(m_Mutex);
            if (!m_Running) return;
            m_Running = false;
            m_Paused = false;
        }
        m_Condition.notify_all();
    }

    void Join() {
        if (m_Thread.joinable() && std::this_thread::get_id() != m_Thread.get_id()) {
            m_Thread.join();
        }
    }

    ~Ticker() {
        Stop();
        if (m_Thread.joinable() && std::this_thread::get_id() == m_Thread.get_id()) {
            m_Thread.detach();
        } else {
            Join();
        }
    }

    Ticker(const std::function<void()>& onTick, const std::chrono::milliseconds interval)
        : m_OnTick(onTick), m_Interval(interval), m_RemainingInterval(interval), m_Running(false), m_Paused(false) {}

    void Start() {
        if (m_Thread.joinable()) {
            if (std::this_thread::get_id() == m_Thread.get_id()) return;
            m_Thread.join();
        }

        std::lock_guard lock(m_Mutex);
        if (m_Running) return;
        m_Running = true;
        m_Paused = false;
        m_RemainingInterval = m_Interval;
        m_Thread = std::thread(&Ticker::RunLoop, this);
    }

    void Pause() {
        {
            std::lock_guard lock(m_Mutex);
            if (!m_Running || m_Paused) {
                return;
            }
            m_Paused = true;
        }
        m_Condition.notify_all();
    }

    void Resume() {
        {
            std::lock_guard lock(m_Mutex);
            if (!m_Running || !m_Paused) {
                return;
            }
            m_Paused = false;
        }
        m_Condition.notify_all();
    }

    void UpdateInterval(std::chrono::milliseconds newInterval) {
        std::lock_guard lock(m_Mutex);
        m_Interval = newInterval;
        if (!m_Paused) {
            m_RemainingInterval = newInterval;
        }
        // m_Condition.notify_all();
    }

    bool isRunning() const { return m_Running; }
};