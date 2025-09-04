#pragma once
#include <queue>
#include <shared_mutex>
#include "CLibUtilsQTR/Ticker.hpp"

struct Animation {
	RE::TESIdleForm* a_idle=nullptr;
    std::string anim_name;
	unsigned int t_wait_ms=0;
	uint32_t anim_id = 0;
};

class Animator:
public Ticker,
public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{

	static bool SendAnimationEvent(RE::Actor* a_actor, const char* AnimationString)
    {
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
			logger::info("Playing animation {} on actor {}", a_animation, a_actor->GetDisplayFullName());
            return SendAnimationEvent(a_actor, a_animation);
        }
        return false;
	}

	bool PlayIdle(RE::TESIdleForm* a_idle, RE::TESObjectREFR* a_target=nullptr) const {
	    if (const auto a_actor = actor.get()) {
            if (const auto current_process = a_actor->GetActorRuntimeData().currentProcess) {
                return current_process->PlayIdle(a_actor,a_idle,a_target);
            }
        }
		return false;
	}

    void UpdateLoop() {
		std::unique_lock lock(animQ_mutex);

		Stop();
		if (m_AnimQueue.empty()) {
			UpdateInterval(std::chrono::milliseconds(0));
			return;
		}

		auto [a_idle, a_anim, t_wait_ms, anim_id] = m_AnimQueue.front();
		m_AnimQueue.pop();
		UpdateInterval(std::chrono::milliseconds(t_wait_ms));
		if (a_idle) {
			SKSE::GetTaskInterface()->AddTask([this, a_idle]() {
				if (PlayIdle(a_idle)) {
					Start();
				}
				else {
					Stop();
					UpdateInterval(std::chrono::milliseconds(10));
					Start();
				}
				});
		}
		else if (!a_anim.empty()) {
			SKSE::GetTaskInterface()->AddTask([this,a_anim]() {
				if (PlayAnimation(a_anim.c_str())) {
					Start();
				}
				else {
					Stop();
					UpdateInterval(std::chrono::milliseconds(10));
					Start();
				}
			});
		}
		else {
			Start();
		}
    }

protected:

	RE::ActorHandlePtr actor;
    std::shared_mutex animQ_mutex;
	std::queue<Animation> m_AnimQueue;

public:
    explicit Animator(RE::ActorHandlePtr a_actor) : Ticker([this]() { UpdateLoop(); },std::chrono::milliseconds(0)),
                                                    actor(std::move(a_actor)) {}

	virtual RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*)=0;

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