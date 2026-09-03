#pragma once
#include <functional>
#include <queue>
#include <shared_mutex>
#include "CLibUtilsQTR/Ticker.hpp"

struct Animation {
    RE::TESIdleForm* a_idle = nullptr;
    RE::ObjectRefHandle target{};
    std::string anim_name;
    unsigned int t_wait_ms = 0;
    uint32_t anim_id = 0;
    std::function<bool(RE::Actor *, Animation &)> before_play{};
};

class Animator : public Ticker, public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    static constexpr auto failure_interval = std::chrono::milliseconds(10);
    bool dispatch_pending{false};
    bool pause_requested{false};

    static bool RunBeforePlay(RE::Actor* a_actor, Animation& a_animation) {
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
        return a_animation.a_idle
                   ? PlayIdle(a_animation.a_idle, a_animation.target.get().get())
                   : PlayAnimation(a_animation.anim_name.c_str());
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
        if (!animation.a_idle && animation.anim_name.empty()) {
            UpdateInterval(std::chrono::milliseconds(animation.t_wait_ms));
            StartIfReady();
            return;
        }

        dispatch_pending = true;
        SKSE::GetTaskInterface()->AddTask([this, animation = std::move(animation)]() mutable {
            if (const auto a_actor = actor.get(); RunBeforePlay(a_actor, animation) && Play(animation)) {
                UpdateInterval(std::chrono::milliseconds(animation.t_wait_ms));
            } else {
                UpdateInterval(failure_interval);
            }

            {
                std::unique_lock lock(animQ_mutex);
                dispatch_pending = false;
                StartIfReady();
            }
        });
    }

protected:
    void StartIfReady() {
        if (!dispatch_pending && !pause_requested) {
            Start();
        }
    }

    RE::ActorHandlePtr actor;
    std::shared_mutex animQ_mutex;
    std::queue<Animation> m_AnimQueue;

public:
    explicit Animator(RE::ActorHandlePtr a_actor)
        : Ticker([this]() { UpdateLoop(); }, std::chrono::milliseconds(0)), actor(std::move(a_actor)) {
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override = 0;

    void Pause() {
        std::unique_lock lock(animQ_mutex);
        pause_requested = true;
        Ticker::Pause();
    }

    void Resume() {
        std::unique_lock lock(animQ_mutex);
        Ticker::Resume();
        if (pause_requested) {
            pause_requested = false;
            StartIfReady();
        }
    }

    void ClearQueue() {
        std::unique_lock lock(animQ_mutex);
        m_AnimQueue = {};
    }

    void Add2Q(const std::vector<Animation>& animations) {
        if (animations.empty()) {
            return;
        }

        std::unique_lock lock(animQ_mutex);
        for (const auto& anim : animations) {
            m_AnimQueue.push(anim);
        }

        StartIfReady();
    }
};