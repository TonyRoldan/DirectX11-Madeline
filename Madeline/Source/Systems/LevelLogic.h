// The level system is responsible for transitioning the various levels in the game
#ifndef LEVELLOGIC_H
#define LEVELLOGIC_H

#include "../GameConfig.h"

#include "../Entities/PlayerData.h"
#include "../Entities/TileData.h"

#include "../Components/Identification.h"
#include "../Components/Gameplay.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Tiles.h"

#include "../Loaders/SaveLoader.h"

#include "../Events/PlayEvents.h"
#include "../Events/GameStateEvents.h"
#include "../Events/EditorEvents.h"
#include "../Events/LevelEvents.h"

// example space game (avoid name collisions)
namespace MAD
{
	class LevelLogic
	{

	private:
		std::shared_ptr<flecs::world> flecsWorld;
		flecs::world flecsWorldAsync;
		GW::CORE::GThreadShared flecsWorldLock;

		std::weak_ptr<const GameConfig> gameConfig;

		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventGenerator editorEventPusher;
		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventResponder playEventHandler;
		GW::CORE::GEventResponder gameStateEventHandler;
		GW::CORE::GEventResponder editorEventHandler;
		GW::CORE::GEventResponder levelEventHandler;

		GW::SYSTEM::GDaemon finishEnterScene{};
		GW::SYSTEM::GDaemon respawnPause{};

		std::shared_ptr<SaveLoader> saveLoader;

		flecs::query<Tile> tileQuery;
		flecs::query<Player> playerQuery;

		GAME_STATE curGameState;
		std::vector<USHORT> curLoadedScenes;
		USHORT nextSceneIndex;
		USHORT curSceneIndex;

		TileData* tileData;

		unsigned sceneExitTime;
		unsigned respawnPauseTime;

	public:
		// attach the required logic to the ECS 
		bool Init(
			std::shared_ptr<flecs::world> _flecsWorld,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			GW::CORE::GEventGenerator _editorTileEventPusher,
			GW::CORE::GEventGenerator _levelEventPusher,
			std::weak_ptr<const GameConfig> _gameConfig,
			std::shared_ptr<SaveLoader> _saveLoader,
			TileData* _tileData);

	private:
		void InitEventHandlers();
		void InitMergeAsyncSystem();

		void OnPlayerDestroyed(PLAY_EVENT_DATA _data);
		
		void OnAddTile(EDITOR_EVENT_DATA _data);
		void OnRemoveTile(EDITOR_EVENT_DATA _data);

		void OnHitSceneExit(LEVEL_EVENT_DATA _data);
		void OnLoadScene(LEVEL_EVENT_DATA _data);
		void OnUnloadScene(LEVEL_EVENT_DATA _data);

		void PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _eventData);

		void SpawnPlayer();

		void SpawnTile(
			const TilemapTile& _tile, 
			std::shared_ptr<Tilemap> _scene, 
			USHORT _sceneIndex, 
			int _sceneRow, 
			int _sceneCol);

		std::string GetTilePrefabName(EDITOR_EVENT_DATA data);
		std::string GetTileName(EDITOR_EVENT_DATA data);
		std::string GetTileName(USHORT _tilesetId, USHORT _orientationId, USHORT _sceneIndex, int _sceneRow, int _sceneCol);

		void AddCurLoadedScene(USHORT _sceneIndex);
		void RemoveCurLoadedScene(USHORT _sceneIndex);
		bool IsSceneLoaded(USHORT _sceneIndex);

		bool CanEnterScene(USHORT _sceneIndex);
		void EnterScene(USHORT _sceneIndex, flecs::entity _sceneExit);
		void LoadScene(USHORT _sceneIndex);
		void LoadSceneNeighbors(USHORT _sceneIndex);
		void UnloadScene(USHORT _sceneIndex);
		void ShowScene(USHORT _sceneIndex);
		void ShowSceneNeighbors(USHORT _sceneIndex);
		void HideScene(USHORT _sceneIndex);
		void HideNonNeighborScenes(USHORT _sceneIndex);
		void DestroyScene(USHORT _sceneIndex);

	

	public:
		void Reset();
		bool Activate(bool runSystem);
		bool Shutdown();
	};

};

#endif