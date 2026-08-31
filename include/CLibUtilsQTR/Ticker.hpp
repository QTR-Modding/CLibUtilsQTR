#pragma once
#include <functional>

class Ticker {
    enum class State {
        kStopped,
        kRunning,
        kPaused,
        kTerminating
    };

    std::function<void()> m_OnTick;
    std::chrono::milliseconds m_Interval;
    std::chrono::milliseconds m_RemainingInterval;

    std::thread m_Thread;
    std::atomic<State> m_State{State::kStopped};
    std::optional<std::chrono::steady_clock::time_point> m_Deadline;

    std::mutex m_Mutex;
    std::condition_variable m_Condition;

    void RunLoop() {
        std::unique_lock lock(m_Mutex);

        while (m_State != State::kTerminating) {
            m_Condition.wait(lock, [this] {
                return m_State == State::kTerminating || m_State == State::kRunning;
            });
            if (m_State == State::kTerminating) break;

            m_Deadline = std::chrono::steady_clock::now() + m_RemainingInterval;
            const auto deadline = *m_Deadline;
            if (m_Condition.wait_until(lock, deadline, [this] {
                    return m_State == State::kTerminating || !m_Deadline;
                })) {
                continue;
            }

            m_Deadline.reset();
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

    Ticker(const Ticker&) = delete;
    Ticker& operator=(const Ticker&) = delete;
    Ticker(Ticker&&) = delete;
    Ticker& operator=(Ticker&&) = delete;

    void Stop() {
        {
            std::lock_guard lock(m_Mutex);
            if (m_State == State::kStopped || m_State == State::kTerminating) return;
            m_State = State::kStopped;
            m_Deadline.reset();
        }
        m_Condition.notify_all();
    }

    void Join() {
        std::thread t;
        {
            std::lock_guard lk(m_Mutex);
            if (!m_Thread.joinable()) return;
            if (std::this_thread::get_id() == m_Thread.get_id()) {
                std::terminate();  // forbid self-join
            }
            m_State = State::kTerminating;
            m_Deadline.reset();
            t = std::move(m_Thread);
        }
        m_Condition.notify_all();
        t.join();

        {
            std::lock_guard lk(m_Mutex);
            m_State = State::kStopped;
        }
    }


    ~Ticker() {
        Join();
    }


    Ticker(const std::function<void()>& onTick, const std::chrono::milliseconds interval)
        : m_OnTick(onTick), m_Interval(interval), m_RemainingInterval(interval) {
    }

    void Start() {
        {
            std::lock_guard lock(m_Mutex);
            if (m_State != State::kStopped) return;

            if (!m_Thread.joinable()) {
                m_Thread = std::thread(&Ticker::RunLoop, this);
            }

            m_RemainingInterval = m_Interval;
            m_State = State::kRunning;
        }

        m_Condition.notify_all();
    }


    void Pause() {
        {
            std::lock_guard lock(m_Mutex);
            if (m_State != State::kRunning) {
                return;
            }
            if (m_Deadline) {
                m_RemainingInterval = std::max(
                    std::chrono::duration_cast<std::chrono::milliseconds>(*m_Deadline -
                                                                          std::chrono::steady_clock::now()),
                    std::chrono::milliseconds(0));
                m_Deadline.reset();
            }
            m_State = State::kPaused;
        }
        m_Condition.notify_all();
    }

    void Resume() {
        {
            std::lock_guard lock(m_Mutex);
            if (m_State != State::kPaused) {
                return;
            }
            m_State = State::kRunning;
        }
        m_Condition.notify_all();
    }

    void UpdateInterval(std::chrono::milliseconds newInterval) {
        std::lock_guard lock(m_Mutex);
        m_Interval = newInterval;
        if (m_State != State::kPaused) {
            m_RemainingInterval = newInterval;
        }
        // m_Condition.notify_all();
    }

    bool isRunning() const {
        const auto state = m_State.load();
        return state == State::kRunning || state == State::kPaused;
    }
};
