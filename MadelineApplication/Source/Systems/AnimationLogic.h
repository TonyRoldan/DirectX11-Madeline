#pragma once
#include "Renderer.h"
#include "../Events/AnimationEvents.h"

namespace MAD
{
	class AnimationLogic
	{	
		DirectX11Renderer* renderer;
		std::shared_ptr<flecs::world> flecsWorld;
		GW::CORE::GEventGenerator animEventPusher;
		GW::CORE::GEventResponder animEventResponder;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventResponder gameStateEventResponder;
		ANIM_EVENT currEvent;

	public:
		void Init(DirectX11Renderer* _renderer,
			std::shared_ptr<flecs::world> _flecsWorld,
			GW::CORE::GEventGenerator _animEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher);
	private:
		void CreateEvents();
		void Transition(PLAYER_ANIMATIONS nextAnim, float transitionLength);

#pragma region Shutdown / Activate
	public:
		bool Activate(bool runSystem);

		bool Shutdown();
#pragma endregion
	};
}