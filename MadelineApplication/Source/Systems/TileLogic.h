#ifndef TILELOGIC_H
#define TILELOGIC_H

#include "../GameConfig.h"

#include "../Loaders/SaveLoader.h"

#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"

#include "../Events/Playevents.h"
#include "../Events/LevelEvents.h"
#include "../Events/GameStateEvents.h"
#include "../Events/TouchEvents.h"

namespace MAD
{
	class TileLogic
	{
		std::shared_ptr<flecs::world> flecsWorld;
		std::weak_ptr<const GameConfig> gameConfig;

		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventGenerator touchEventPusher;
		GW::CORE::GEventResponder playEventHandler;
		GW::CORE::GEventResponder touchEventHandler;

		GW::SYSTEM::GDaemon winPause{};

		std::shared_ptr<SaveLoader> saveLoader;

		flecs::query<Player, Moveable> playerQuery;
		flecs::query<Strawberry, FollowPlayer> followingStrawberriesQuery;

		flecs::system springSystem;
		flecs::system crystalCollectSystem;
		flecs::system crystalRespawnSystem;
		flecs::system spikeSystem;
		flecs::system strawberrySystem;
		flecs::system graveSystem;
		flecs::system sceneExitSystem;
		flecs::system crumblingPlatformCrumbleSystem;
		flecs::system crumblingPlatformRespawnSystem;

		float strawberryFollowDist;
		float strawberryFollowSmoothing;

		unsigned crystalRespawnTime;

		unsigned crumblingPlatformCrumbleTime;
		unsigned crumblingPlatformRespawnTime;

	public:
		bool Init(
			std::shared_ptr<flecs::world> _flecsWorld,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _levelEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			GW::CORE::GEventGenerator _touchEventPusher,
			std::shared_ptr<SaveLoader> _saveLoader);

	private:

		void InitEventHandlers();
		void InitSpringSystem();
		void InitCrystalSystems();
		void InitSpikeSystem();
		void InitStrawberrySystem();
		void InitGraveSystem();
		void InitSceneExitSystem();
		void InitCrumblingPlatformSystems();

		void PushPlayEvent(PlayEvent _event, PLAY_EVENT_DATA _data = {});
		void PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _data = {});
		void PushGameStateEvent(GAME_STATE _event, GAME_STATE_EVENT_DATA _data = {});

		void OnPlayerDestroyed(PLAY_EVENT_DATA _data);
		void OnCollectStrawberries(PLAY_EVENT_DATA _data);
		void OnCollectCrystal(PLAY_EVENT_DATA _data);

		void OnEnterTouch(TouchEventData _data);
		void OnExitTouch(TouchEventData _data);

	public:
		bool Activate(bool runSystem);
		bool Shutdown();
	};
};

#endif