#pragma once
#include <functional>
#include <queue>
#include <shared_mutex>
#include "CLibUtilsQTR/Ticker.hpp"

struct Animation {
    RE::TESIdleForm* a_idle = nullptr;
    std::string anim_name;
    unsigned int t_wait_ms = 0;
    uint32_t anim_id = 0;
    std::function<bool(RE::Actor*, Animation&)> before_play{};
};

class Animator :
    public Ticker,
    public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    std::atomic_bool play_pending{false};

    void ContinueQueue() {
        play_pending = false;
        Start();
    }

    bool RunBeforePlay(RE::Actor* a_actor, Animation& a_animation) {
        if (!a_actor) {
            return false;
        }
        try {
            if (a_animation.before_play && !a_animation.before_play(a_actor, a_animation)) return false;
            UpdateInterval(std::chrono::milliseconds(a_animation.t_wait_ms));
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool SendAnimationEvent(RE::Actor* a_actor, const char* AnimationString) {
        if (const auto animGraphHolder = static_cast<RE::IAnimationGraphManagerHolder*>(a_actor)) {
            if (animGraphHolder->NotifyAnimationGraph(AnimationString)) {
                return true;
            }
            return false;
        }
        return false;
    }

    bool PlayAnimation(const char* a_animation) {
        if (const auto a_actor = actor.get()) {
            a_actor->AddAnimationGraphEventSink(this);
            return SendAnimationEvent(a_actor, a_animation);
        }
        return false;
    }

    bool PlayIdle(RE::TESIdleForm* a_idle, RE::TESObjectREFR* a_target = nullptr) const {
        if (const auto a_actor = actor.get()) {
            if (const auto current_process = a_actor->GetActorRuntimeData().currentProcess) {
                return current_process->PlayIdle(a_actor, a_idle, a_target);
            }
        }
        return false;
    }

    void UpdateLoop() {
        std::unique_lock lock(animQ_mutex);

        if (m_AnimQueue.empty()) {
            Stop();
            UpdateInterval(std::chrono::milliseconds(0));
            return;
        }

        auto animation = m_AnimQueue.front();
        m_AnimQueue.pop();
        if (animation.a_idle || !animation.anim_name.empty()) {
            play_pending = true;
            Stop();
        }
        if (animation.a_idle) {
            SKSE::GetTaskInterface()->AddTask([this, animation = std::move(animation)]() mutable {
                const auto a_actor = actor.get();
                if (RunBeforePlay(a_actor, animation) && PlayIdle(animation.a_idle)) {
                    ContinueQueue();
                } else {
                    UpdateInterval(std::chrono::milliseconds(10));
                    ContinueQueue();
                }
            });
        } else if (!animation.anim_name.empty()) {
            SKSE::GetTaskInterface()->AddTask([this, animation = std::move(animation)]() mutable {
                const auto a_actor = actor.get();
                if (RunBeforePlay(a_actor, animation) && PlayAnimation(animation.anim_name.c_str())) {
                    ContinueQueue();
                } else {
                    UpdateInterval(std::chrono::milliseconds(10));
                    ContinueQueue();
                }
            });
        } else {
            UpdateInterval(std::chrono::milliseconds(animation.t_wait_ms));
        }
    }

protected:
    RE::ActorHandlePtr actor;
    std::shared_mutex animQ_mutex;
    std::queue<Animation> m_AnimQueue;

public:
    void Start() {
        if (!play_pending) Ticker::Start();
    }

    explicit Animator(RE::ActorHandlePtr a_actor) : Ticker([this]() { UpdateLoop(); }, std::chrono::milliseconds(0)),
                                                    actor(std::move(a_actor)) {
    }

    virtual RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                  RE::BSTEventSource<RE::BSAnimationGraphEvent>*) =0;

    void ClearQueue() {
        Stop();
        UpdateInterval(std::chrono::milliseconds(0));
        std::unique_lock lock(animQ_mutex);
        m_AnimQueue = std::queue<Animation>();
    }

    void Add2Q(const std::vector<Animation>& animations) {
        if (animations.empty()) {
            return;
        }

        std::unique_lock lock(animQ_mutex);
        for (const auto& anim : animations) {
            m_AnimQueue.push(anim);
        }

        Start();
    }
};
