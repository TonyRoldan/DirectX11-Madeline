#pragma once

#include "../Loaders/ModelLoader.h"
#include "../GameConfig.h"

// example space game (avoid name collisions)
namespace MAD
{
	enum TilesetId
	{
		SPRING_ID = 5,
		SCENE_EXIT_ID = 6,
		SPAWNPOINT_ID = 7,
		CRYSTAL_ID = 8,
		STRAWBERRY_ID = 9,
		GRAVE_ID = 10,
		SPIKES_ID = 11,
		PLATFORM_ID = 12,
		CRUMBLING_PLATFORM_ID = 13
	};

	class TileData
	{
		std::weak_ptr<const GameConfig> gameConfig;

	public:
		std::vector<std::string> tilesetNames;
		std::map<std::string, unsigned> tilesetNameIndices;
		std::vector<std::vector<unsigned>> tileModelIndices;
		std::map<std::string, std::vector<std::string>> tilePrefabNames;

		std::string GetTilePrefabName(unsigned _tilesetId, unsigned _orientationId);
		unsigned GetTileModelIndex(unsigned _tilesetId, unsigned _orientationId);
		bool IsIdOfTileset(unsigned _tilesetId, std::string _tilesetName);

		void Init(
			std::shared_ptr<flecs::world> _flecsWorld,
			std::weak_ptr<const GameConfig> _gameConfig);

		// Load required entities and/or prefabs into the ECS 
		bool Load(
			std::shared_ptr<flecs::world> _flecsWorld,
			unsigned int _modelIndex,
			std::string _tilesetName);

		// Unload the entities/prefabs from the ECS
		bool Unload(std::shared_ptr<flecs::world> _flecsWorld);

		bool HasComponent(std::string _tilesetName, std::string _componentName);
	};

};