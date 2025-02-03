#include <random>
#include "LevelLogic.h"

#include "../Components/Gameplay.h"
#include "../Components/AudioSource.h"

#include "../Entities/Prefabs.h"

using namespace MAD;
using namespace flecs;
using namespace GW;
using namespace CORE;
using namespace MATH;

#pragma region Init
bool LevelLogic::Init(
	std::shared_ptr<world> _flecsWorld,
	GEventGenerator _playEventPusher,
	GW::CORE::GEventGenerator _gameStateEventPusher,
	GEventGenerator _editorTileEventPusher,
	GEventGenerator _levelEventPusher,
	std::weak_ptr<const GameConfig> _gameConfig,
	std::shared_ptr<SaveLoader> _saveLoader,
	TileData* _tileData)
{
	flecsWorld = _flecsWorld;

	flecsWorldAsync = flecsWorld->async_stage();
	flecsWorldLock.Create();
	gameConfig = _gameConfig;
	playEventPusher = _playEventPusher;
	gameStateEventPusher = _gameStateEventPusher;
	editorEventPusher = _editorTileEventPusher;
	levelEventPusher = _levelEventPusher;

	saveLoader = _saveLoader;

	tileData = _tileData;

	tileQuery = flecsWorld->query<Tile>();
	playerQuery = flecsWorld->query<Player>();

	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();

	sceneExitTime = readCfg->at("SceneExit").at("exitTime").as<unsigned>();
	respawnPauseTime = readCfg->at("Spawnpoint").at("respawnPauseTime").as<unsigned>();

	InitEventHandlers();
	InitMergeAsyncSystem();

	return true;
}
#pragma endregion

#pragma region Event Handlers
void MAD::LevelLogic::InitEventHandlers()
{
	playEventHandler.Create([this](const GW::GEvent& _event)
		{
			PlayEvent event;
			PLAY_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case PlayEvent::PLAYER_DESTROYED:
			{
				OnPlayerDestroyed(data);
				break;
			}
			default:
			{
				break;
			}
			}
		});
	playEventPusher.Register(playEventHandler);

	gameStateEventHandler.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE event;
			GAME_STATE_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			curGameState = event;
		});
	gameStateEventPusher.Register(gameStateEventHandler);

	editorEventHandler.Create([this](const GW::GEvent& _event)
		{
			EDITOR_EVENT event;
			EDITOR_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case EDITOR_EVENT::ADD_TILE:
			{
				OnAddTile(data);
				break;
			}
			case EDITOR_EVENT::REMOVE_TILE:
			{
				OnRemoveTile(data);
				break;
			}
			default:
			{
				break;
			}
			}
		});
	editorEventPusher.Register(editorEventHandler);

	levelEventHandler.Create([this](const GW::GEvent& _event)
		{
			LEVEL_EVENT event;
			LEVEL_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case LEVEL_EVENT::HIT_SCENE_EXIT:
			{
				OnHitSceneExit(data);
				break;
			}
			case LEVEL_EVENT::LOAD_SCENE:
			{
				OnLoadScene(data);
				break;
			}
			case LEVEL_EVENT::UNLOAD_SCENE:
			{
				OnUnloadScene(data);
				break;
			}
			default:
			{
				break;
			}
			}
		});
	levelEventPusher.Register(levelEventHandler);
}
#pragma endregion

#pragma region Init Systems
void MAD::LevelLogic::InitMergeAsyncSystem()
{
	struct MergeLevelStages {};
	flecsWorld->entity("MergeLevelStages").add<MergeLevelStages>();
	flecsWorld->system<MergeLevelStages>()
		.kind(OnLoad)
		.each([this](entity _entity, MergeLevelStages& _MergeLevelStages)
			{
				flecsWorldLock.LockSyncWrite();
				flecsWorldAsync.merge();
				flecsWorldLock.UnlockSyncWrite();
			});
}
#pragma endregion

#pragma region Play Events
void MAD::LevelLogic::OnPlayerDestroyed(PLAY_EVENT_DATA _data)
{
	respawnPause.Create(1, [&]()
		{
			std::shared_ptr<Tilemap> scene = saveLoader->GetScene(saveLoader->GetSaveSlot().sceneIndex);
			const Spawnpoint* spawnpoint = scene->GetSpawnpointByScene(saveLoader->GetSaveSlot().prevSceneIndex);

			GMATRIXF transform = playerQuery.first().get<Transform>()->value;
			if (spawnpoint != NULL)
			{
				transform.row4 = spawnpoint->position;
				transform.row4.y += .1;
			}
			else
				transform.row4 = { 20, 5 };
			transform.row4.w = 1;

			playerQuery.first().set<Transform>({ transform });
			playerQuery.first().add<RenderModel>();
			playerQuery.first().add<Collidable>();
			playerQuery.first().add<Moveable>();

			respawnPause.Pause(0, false);
		}, respawnPauseTime);
}
#pragma endregion

#pragma region Editor Events
void MAD::LevelLogic::OnAddTile(EDITOR_EVENT_DATA _data)
{
	TilemapTile tile = { _data.tileset, _data.orientation };
	SpawnTile(
		tile,
		saveLoader->GetScene(_data.sceneIndex),
		_data.sceneIndex,
		_data.sceneRow,
		_data.sceneCol);
}

void MAD::LevelLogic::OnRemoveTile(EDITOR_EVENT_DATA _data)
{
	std::string name = GetTileName(_data);
	if (!name.empty())
		flecsWorld->entity(name.c_str()).destruct();
}
#pragma endregion

#pragma region Level Events
void MAD::LevelLogic::OnHitSceneExit(LEVEL_EVENT_DATA _data)
{
	EnterScene((USHORT)_data.sceneIndex, _data.sceneExit);
}

void MAD::LevelLogic::OnLoadScene(LEVEL_EVENT_DATA _data)
{
	ShowScene(_data.sceneIndex);
}

void MAD::LevelLogic::OnUnloadScene(LEVEL_EVENT_DATA _data)
{
	// TODO implement
}
#pragma endregion

#pragma region Event Pushers
void MAD::LevelLogic::PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _eventData)
{
	GW::GEvent errorEvent;
	errorEvent.Write(_event, _eventData);
	levelEventPusher.Push(errorEvent);
}
#pragma endregion

#pragma region Player
void LevelLogic::SpawnPlayer()
{
	flecs::entity playerPrefab{};
	if (RetreivePrefab("Player", playerPrefab))
	{
		std::string name = "Player." + std::to_string(saveLoader->GetSaveSlot().deaths);

		std::shared_ptr<Tilemap> scene = saveLoader->GetScene(saveLoader->GetSaveSlot().sceneIndex);
		const Spawnpoint* spawnpoint = scene->GetSpawnpointByScene(saveLoader->GetSaveSlot().prevSceneIndex);

		GMATRIXF transform = playerPrefab.get<Transform>()->value;
		if (spawnpoint != NULL)
		{
			transform.row4 = spawnpoint->position;
			transform.row4.y += .1;
		}
		else
			transform.row4 = { 20, 5 };
		transform.row4.w = 1;

		flecsWorldLock.LockSyncWrite();
		flecs::entity spawnedPlayer = flecsWorldAsync.entity(name.c_str()).is_a(playerPrefab)
			.add<Moveable>()
			.add<Collidable>()
			.set<Transform>({ transform })
			.set<ControllerID>({ 0 })
			.add<RenderModel>();

		ColliderContainer colliders(*playerPrefab.get<ColliderContainer>(), spawnedPlayer, transform.row4);

		spawnedPlayer.set<ColliderContainer>(colliders);
		flecsWorldLock.UnlockSyncWrite();

	}

}
#pragma endregion

#pragma region Tiles
void MAD::LevelLogic::SpawnTile(
	const TilemapTile& _tile,
	std::shared_ptr<Tilemap> _scene,
	USHORT _sceneIndex,
	int _sceneRow,
	int _sceneCol)
{
	flecs::entity tilePrefab{};
	std::string prefabName = tileData->GetTilePrefabName(_tile.tilesetId, _tile.orientationId);
	if (RetreivePrefab(prefabName.c_str(), tilePrefab))
	{
		std::string tileName = GetTileName(_tile.tilesetId, _tile.orientationId, _sceneIndex, _sceneRow, _sceneCol);

		GMATRIXF transform = tilePrefab.get<Transform>()->value;
		transform.row4.x += _scene->originX + _sceneCol;
		transform.row4.y += _scene->originY + _sceneRow;

		Tile tileInfo = { _sceneIndex, (USHORT)_sceneRow, (USHORT)_sceneCol };

		flecsWorldLock.LockSyncWrite();
		flecs::entity spawnedTile = flecsWorldAsync.entity(tileName.c_str()).is_a(tilePrefab)
			.set<Transform>({ transform })
			.set<Tile>(tileInfo);

		if (_tile.tilesetId == STRAWBERRY_ID)
		{
			spawnedTile.set<Strawberry>({ transform.row4 });

			if (saveLoader->GetSaveSlot().IsStrawberryCollected(_sceneIndex))
			{
				spawnedTile.add<Collected>();
				spawnedTile.add<RenderInEditor>();

				if (curGameState == LEVEL_EDITOR)
					spawnedTile.add<RenderModel>();
			}
			else
			{
				spawnedTile.add<Collidable>();
				spawnedTile.add<RenderModel>();
			}
		}
		else
		{
			spawnedTile.add<Collidable>();

			if (!tilePrefab.has<RenderInEditor>() || curGameState == LEVEL_EDITOR)
				spawnedTile.add<RenderModel>();
		}

		ColliderContainer colliders(*tilePrefab.get<ColliderContainer>(), spawnedTile, transform.row4);

		spawnedTile.set<ColliderContainer>(colliders);
		flecsWorldLock.UnlockSyncWrite();
	}
}

std::string MAD::LevelLogic::GetTilePrefabName(EDITOR_EVENT_DATA data)
{
	return tileData->GetTilePrefabName(data.tileset, data.orientation);
}

std::string MAD::LevelLogic::GetTileName(EDITOR_EVENT_DATA data)
{
	return GetTileName(data.tileset, data.orientation, data.sceneIndex, data.sceneRow, data.sceneCol);
}

std::string MAD::LevelLogic::GetTileName(
	USHORT _tilesetId,
	USHORT _orientationId,
	USHORT _sceneIndex,
	int _sceneRow,
	int _sceneCol)
{
	// Name format:
	// name.plannerX.plannerY.mapX.mapY
	std::string name = tileData->GetTilePrefabName(_tilesetId, _orientationId);
	name += "." + std::to_string(_sceneIndex) + "." +
		std::to_string(_sceneRow) + "." + std::to_string(_sceneCol);
	return name;
}

void MAD::LevelLogic::AddCurLoadedScene(USHORT _sceneIndex)
{
	if (curLoadedScenes.size() == 0)
	{
		nextSceneIndex = _sceneIndex;
		curSceneIndex = _sceneIndex;
	}

	auto loadedScene = std::find(curLoadedScenes.begin(), curLoadedScenes.end(), _sceneIndex);

	if (loadedScene == curLoadedScenes.end())
		curLoadedScenes.push_back(_sceneIndex);
}

void MAD::LevelLogic::RemoveCurLoadedScene(USHORT _sceneIndex)
{
	auto loadedScene = std::find(curLoadedScenes.begin(), curLoadedScenes.end(), _sceneIndex);

	if (loadedScene != curLoadedScenes.end())
		curLoadedScenes.erase(loadedScene);
}

bool MAD::LevelLogic::IsSceneLoaded(USHORT _sceneIndex)
{
	return std::find(curLoadedScenes.begin(), curLoadedScenes.end(), _sceneIndex) != curLoadedScenes.end();
}
#pragma endregion

#pragma region Scenes
bool MAD::LevelLogic::CanEnterScene(USHORT _sceneIndex)
{
	return _sceneIndex != curSceneIndex && nextSceneIndex == curSceneIndex;
}

void MAD::LevelLogic::EnterScene(USHORT _sceneIndex, flecs::entity _sceneExit)
{
	if (!CanEnterScene(_sceneIndex))
		return;

	nextSceneIndex = _sceneIndex;

	ShowScene(nextSceneIndex);
	ShowSceneNeighbors(nextSceneIndex);

	saveLoader->EnterScene(nextSceneIndex, curSceneIndex);
	PushLevelEvent(ENTER_SCENE_BEGIN, { nextSceneIndex, _sceneExit });

	HideNonNeighborScenes(nextSceneIndex);

	finishEnterScene.Create(1, [&]()
		{
			curSceneIndex = nextSceneIndex;
			PushLevelEvent(ENTER_SCENE_DONE, { curSceneIndex });
			finishEnterScene.Pause(0, false);
		}, sceneExitTime);
}

void MAD::LevelLogic::LoadScene(USHORT _sceneIndex)
{
	if (IsSceneLoaded(_sceneIndex))
		return;

	const std::shared_ptr<Tilemap> tilemap = saveLoader->GetScene(_sceneIndex);

	if (tilemap == NULL)
		return;

	for (int row = 0; row < tilemap->rows; row++)
	{
		for (int col = 0; col < tilemap->columns; col++)
		{
			const TilemapTile& tile = tilemap->tiles[row][col];
			if (tile.tilesetId == 0)
				continue;

			SpawnTile(tile, tilemap, _sceneIndex, row, col);
		}
	}

	AddCurLoadedScene(_sceneIndex);
	PushLevelEvent(LOAD_SCENE_DONE, { _sceneIndex });
}

void MAD::LevelLogic::LoadSceneNeighbors(USHORT _sceneIndex)
{
	std::shared_ptr<Tilemap> scene = saveLoader->GetScene(_sceneIndex);
	if (scene == NULL)
		return;

	for (int i = 0; i < scene->neighborScenes.size(); i++)
		LoadScene(scene->neighborScenes[i]);
}

void MAD::LevelLogic::UnloadScene(USHORT _sceneIndex)
{
	DestroyScene(_sceneIndex);
	RemoveCurLoadedScene(_sceneIndex);
	PushLevelEvent(UNLOAD_SCENE_DONE, { _sceneIndex });
}

void MAD::LevelLogic::ShowScene(USHORT _sceneIndex)
{
	if (IsSceneLoaded(_sceneIndex))
	{
		tileQuery.each([&](entity _entity, Tile& _tile)
			{
				if (_tile.sceneIndex == _sceneIndex)
				{
					if (_entity.has<Strawberry>() && (_entity.has<Collected>() || _entity.has<FollowPlayer>()))
						return;
					if (_entity.has<ColliderContainer>())
						_entity.add<Collidable>();
					if (!_entity.has<RenderInEditor>() || curGameState == LEVEL_EDITOR)
						_entity.add<RenderModel>();
				}
			});

		PushLevelEvent(SHOW_SCENE, { _sceneIndex });
	}
	else
	{
		LoadScene(_sceneIndex);
	}
}

void MAD::LevelLogic::ShowSceneNeighbors(USHORT _sceneIndex)
{
	std::shared_ptr<Tilemap> scene = saveLoader->GetScene(_sceneIndex);
	for (int i = 0; i < scene->neighborScenes.size(); i++)
	{
		ShowScene(scene->neighborScenes[i]);
	}
}

void MAD::LevelLogic::HideScene(USHORT _sceneIndex)
{
	tileQuery.each([&](entity _entity, Tile& _tile)
		{
			if (_tile.sceneIndex == _sceneIndex)
			{
				if (_entity.has<Strawberry>() && _entity.has<FollowPlayer>())
					return;
				if (_entity.has<Collidable>())
					_entity.remove<Collidable>();
				if (_entity.has<RenderModel>())
					_entity.remove<RenderModel>();
			}
		});

	PushLevelEvent(HIDE_SCENE, { _sceneIndex });
}

void MAD::LevelLogic::HideNonNeighborScenes(USHORT _sceneIndex)
{
	std::shared_ptr<Tilemap> scene = saveLoader->GetScene(_sceneIndex);

	for (int i = 0; i < curLoadedScenes.size(); i++)
	{
		if (curLoadedScenes[i] == _sceneIndex)
			continue;

		auto neighbor = std::find(scene->neighborScenes.begin(), scene->neighborScenes.end(), curLoadedScenes[i]);
		if (neighbor == scene->neighborScenes.end())
		{
			HideScene(curLoadedScenes[i]);
		}
	}
}

void MAD::LevelLogic::DestroyScene(USHORT _sceneIndex)
{
	std::vector<entity> tiles;
	tileQuery.each([&](entity _entity, Tile& _tile)
		{
			if (_tile.sceneIndex == _sceneIndex)
			{
				tiles.push_back(_entity);
			}
		});

	for (int i = 0; i < tiles.size(); i++)
	{
		tiles[i].destruct();
	}
}
#pragma endregion

void MAD::LevelLogic::Reset()
{
	if (saveLoader->GetScene(saveLoader->GetSaveSlot().sceneIndex) == NULL)
	{
		std::cout << "No scenes exist\n";
		return;
	}
	curSceneIndex = saveLoader->GetSaveSlot().sceneIndex;
	nextSceneIndex = saveLoader->GetSaveSlot().sceneIndex;
	curLoadedScenes.clear();

	ShowScene(curSceneIndex);
	ShowSceneNeighbors(curSceneIndex);
	SpawnPlayer();
}

// Free any resources used to run this system
bool LevelLogic::Shutdown()
{
	flecsWorldAsync.merge();
	flecsWorld->entity("MergeLevelStages").destruct();
	flecsWorld->entity("Level System").destruct();

	tileQuery.destruct();
	playerQuery.destruct();

	flecsWorld.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool LevelLogic::Activate(bool _runSystem)
{
	if (_runSystem)
	{
		flecsWorld->entity("MergeLevelStages").enable();
	}
	else
	{
		flecsWorld->entity("MergeLevelStages").disable();
	}

	return false;
}

// **** SAMPLE OF MULTI_THREADED USE ****
//world world; // main world
//world async_stage = world.async_stage();
//
//// From thread
//lock(async_stage_lock);
//entity e = async_stage.entity().child_of(parent)...
//unlock(async_stage_lock);
//
//// From main thread, periodic
//lock(async_stage_lock);
//async_stage.merge(); // merge all commands to main world
//unlock(async_stage_lock);