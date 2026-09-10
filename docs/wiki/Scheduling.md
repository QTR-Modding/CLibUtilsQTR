# Scheduling

## Delayed background work

`Tasker` runs callbacks on worker threads:

```cpp
#include <atomic>
#include <memory>
#include <CLibUtilsQTR/Tasker.hpp>

auto completed = std::make_shared<std::atomic<bool>>(false);
constexpr int delayMilliseconds = 100;
clib_utilsQTR::Tasker::GetSingleton()->PushTask(
    [completed] { completed->store(true); }, delayMilliseconds);
```

The shared pointer keeps the example's state alive until completion. `PushTask()` starts the pool if needed. Delays use real milliseconds, not Skyrim game time, and are scheduling targets rather than exact execution times. Only the base package is needed.

Callbacks do not run on Skyrim's game thread. Dispatch game-thread operations through SKSE's task interface, keeping referenced objects alive appropriately.

| Operation | Meaning |
| --- | --- |
| `Start(thread_count)` | Starts a stopped pool; pass a positive count |
| `PushTask(callback, delay_ms, args...)` | Queues delayed work with optional bound arguments |
| `HasTask()` | Reports queued work; an executing callback has already left the queue |
| `IsRunning()` | Reports the pool's running flag |
| `Stop()` | Stops and joins workers; queued work is drained, not cancelled |

Call `Stop()` from the owning thread, not a worker callback. Prevent producers from adding work during shutdown.

`PushSustainedTask(condition, duration_ms, callback, poll_interval_ms)` samples a condition repeatedly. If every sample is true through the duration, it calls the callback once. A false sample ends that chain. The default sampling interval is 50 ms; changes between samples are not observed. Both functions execute on workers.

## Repeating work

`Ticker` owns a worker thread for one repeating callback. It is in the global namespace:

```cpp
#include <atomic>
#include <chrono>
#include <CLibUtilsQTR/Ticker.hpp>

class Counter {
public:
    Counter()
        : ticker([this] { ticks.fetch_add(1); },
                 std::chrono::milliseconds(100)) {}
    void Start() { ticker.Start(); }
    ~Counter() { ticker.Join(); }

private:
    std::atomic<unsigned> ticks{0};
    Ticker ticker;
};
```

Keep the owner alive while ticking is needed. This example joins before destroying the state used by the callback.

| Operation | Meaning |
| --- | --- |
| `Start()` | Starts ticking; the first callback follows the interval |
| `Stop()` | Stops scheduling; an already-running callback may still finish |
| `Join()` | Stops and joins the worker |
| `Pause()` / `Resume()` | Preserve and resume the remaining interval |
| `UpdateInterval(duration)` | Changes the interval without restarting an already-waiting deadline |
| `isRunning()` | True while running or paused |

Do not call `Join()` from the ticker's callback. A callback exception stops the ticker. Like Tasker, Ticker provides background scheduling, not an SKSE task or game-time timer.
