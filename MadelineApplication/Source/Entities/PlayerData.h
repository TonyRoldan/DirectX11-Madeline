// This class populates all player entities 
#pragma once
#include "../Loaders/ModelLoader.h"
#include "../Loaders/AudioLoader.h"
#include "../GameConfig.h"

// example space game (avoid name collisions)
namespace MAD
{
	class PlayerData
	{
	public:
		// Load required entities and/or prefabs into the ECS 
		bool Load(	
			std::shared_ptr<flecs::world> _flecsWorld,
			std::weak_ptr<const GameConfig> _gameConfig,
			unsigned int _modelIndex,
			Model& _model,
			AudioLoader* _audioLoader);

		// Unload the entities/prefabs from the ECS
		bool Unload(std::shared_ptr<flecs::world> _flecsWorld);
	};

};