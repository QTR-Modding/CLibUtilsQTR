# Animations

`Animator` queues idle animations, animation graph events, and waits for one actor. Derive from it and implement its event handler:

```cpp
#include <CLibUtilsQTR/Animations.hpp>

class MyAnimator final : public Animator {
public:
    using Animator::Animator;

    RE::BSEventNotifyControl ProcessEvent(
        const RE::BSAnimationGraphEvent*,
        RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override {
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

Construct a persistent animator using the `RE::ActorHandlePtr` expected by its constructor. Queue a wait with `animator.Add2Q({Animation{.t_wait_ms = 500}});`, or supply a valid idle or graph event for that actor.

Use `skyrim` and the engine PCH. `Animation` and `Animator` are in the global namespace.

| Entry field | Meaning |
| --- | --- |
| `a_idle` | Idle form to play |
| `target` | Reference handle used as idle target |
| `anim_name` | Graph event when no idle is provided |
| `t_wait_ms` | Real-time wait before advancing |
| `anim_id` | Caller-supplied identifier |
| `before_play` | Callback receiving actor and mutable entry; false skips playback |

An entry with neither idle nor graph event is a wait. Its duration does not automatically track animation completion.

`Add2Q()` appends and starts processing. `Pause()` and `Resume()` control scheduling. `ClearQueue()` removes waiting entries; it does not cancel playback already dispatched to SKSE.

The ticker schedules in the background; playback and `before_play` run through an SKSE task. Keep the animator alive while tasks can access it, and manage its animation event sink registration during teardown. A short-lived local animator is not a suitable owner for asynchronous playback.
