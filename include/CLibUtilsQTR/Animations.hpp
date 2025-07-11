#pragma once
#include <queue>
#include <shared_mutex>
#include "CLibUtilsQTR/Ticker.hpp"

struct Animation {
	RE::TESIdleForm* a_idle;
    std::string anim_name;
	int t_wait_ms;
};

class Animator:
public Ticker,
public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
    virtual RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                          RE::BSTEventSource<RE::BSAnimationGraphEvent>*)=0;

	bool SendAnimationEvent(RE::Actor* a_actor, const char* AnimationString)
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
	    const auto player = RE::PlayerCharacter::GetSingleton();
	    player->AddAnimationGraphEventSink(this);
	    return SendAnimationEvent(player, a_animation);
	}

	static void PlayIdle(RE::TESIdleForm* a_idle, RE::TESObjectREFR* a_target=nullptr) {
	    const auto player = RE::PlayerCharacter::GetSingleton();
        player->GetActorRuntimeData().currentProcess->PlayIdle(player,a_idle,a_target);
	}

    void UpdateLoop() {
		std::unique_lock lock(animQ_mutex);

		Stop();
		if (m_AnimQueue.empty()) {
			UpdateInterval(std::chrono::milliseconds(0));
			return;
		}

		auto [a_idle, a_anim, t_wait_ms] = m_AnimQueue.front();
		m_AnimQueue.pop();
		UpdateInterval(std::chrono::milliseconds(t_wait_ms));
		if (a_idle) {
			SKSE::GetTaskInterface()->AddTask([this, a_idle]() {
				PlayIdle(a_idle);
				Start();
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

	std::queue<Animation> m_AnimQueue;
    std::shared_mutex animQ_mutex;

public:
    Animator() : Ticker([this]() { UpdateLoop(); },std::chrono::milliseconds(0)) {}

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