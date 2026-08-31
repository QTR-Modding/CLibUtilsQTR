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
    std::function<bool(RE::Actor*, const Animation&)> before_play{};
};

class Animator :
    public Ticker,
    public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    static constexpr auto failure_interval = std::chrono::milliseconds(10);
    bool dispatch_pending{false};

    static bool RunBeforePlay(RE::Actor* a_actor, const Animation& a_animation) {
        if (!a_actor) {
            return false;
        }
        if (!a_animation.before_play) {
            return true;
        }
        try {
            return a_animation.before_play(a_actor, a_animation);
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

    bool Play(const Animation& a_animation) {
        return a_animation.a_idle ? PlayIdle(a_animation.a_idle) : PlayAnimation(a_animation.anim_name.c_str());
    }

    void UpdateLoop() {
        std::unique_lock lock(animQ_mutex);

        Stop();
        if (m_AnimQueue.empty()) {
            UpdateInterval(std::chrono::milliseconds(0));
            return;
        }

        auto animation = m_AnimQueue.front();
        m_AnimQueue.pop();
        UpdateInterval(std::chrono::milliseconds(animation.t_wait_ms));
        if (!animation.a_idle && animation.anim_name.empty()) {
            Start();
            return;
        }

        dispatch_pending = true;
        SKSE::GetTaskInterface()->AddTask([this, animation = std::move(animation)]() {
            const auto a_actor = actor.get();
            if (!RunBeforePlay(a_actor, animation) || !Play(animation)) {
                Stop();
                UpdateInterval(failure_interval);
            }
            {
                std::unique_lock lock(animQ_mutex);
                dispatch_pending = false;
            }
            Start();
        });
    }

protected:
    RE::ActorHandlePtr actor;
    std::shared_mutex animQ_mutex;
    std::queue<Animation> m_AnimQueue;

public:
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

        if (!dispatch_pending) {
            Start();
        }
    }
};
