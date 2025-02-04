#include "PlayerData.h"
#include "Prefabs.h"
#include "../Utils/Macros.h"
#include "../Components/Gameplay.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/AudioSource.h"
#include "../Components/HapticSource.h"
#include "../Components/Lights.h"

using namespace GW;
using namespace MATH;
using namespace MATH2D;

bool MAD::PlayerData::Load(
	std::shared_ptr<flecs::world> _flecsWorld,
	std::weak_ptr<const GameConfig> _gameConfig,
	unsigned int _modelIndex,
	Model& _model,
	AudioLoader* _audioLoader)
{
	// Perform any necessary operations with the config info to build the player prefab.

	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	if (readCfg->find("Player") == readCfg->end())
		return false;

	// Transform
	Transform transform("Player", _gameConfig);
	ModelOffset modelOffset("Player", _gameConfig);

	// Colliders
	ColliderContainer colliders(true, "Player", _gameConfig);

	// Audio
	SoundClips soundClips("Player", _gameConfig, _audioLoader);
	LoopingClips loopingClips("Player", _gameConfig, _audioLoader);

	// Light
	PointLight light("Player", _gameConfig);

	// Haptics
	Haptics haptics{};
	haptics.info.insert({PLAYER_DEATH, 
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("playerDeathHaptics").as<std::string>()))});
	haptics.info.insert({ LAND_GROUND,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("landGroundHaptics").as<std::string>())) });
	haptics.info.insert({ JUMP,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("jumpHaptics").as<std::string>())) });
	haptics.info.insert({ DASH,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("dashHaptics").as<std::string>())) });
	haptics.info.insert({ WALL_CLIMB,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("wallClimbHaptics").as<std::string>())) });
	haptics.info.insert({ WALL_SLIDE,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("wallSlideHaptics").as<std::string>())) });
	haptics.info.insert({ WALL_SLIDE_NO_STAMINA,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("wallSlideNoStaminaHaptics").as<std::string>())) });
	haptics.info.insert({ SPRING_BOUNCE,
		HapticInfo(StringToGVector(readCfg->at("Haptics").at("sprintBounceHaptics").as<std::string>())) });

	// Prefab
	auto newPrefab = _flecsWorld->prefab("Player")
		.add<Player>()
		.add<Triggerable>()
		.add<PhysicsCollidable>()
		.override<ControllerID>()
		.set_override<Transform>(transform)
		.set_override<ColliderContainer>(colliders)
		.set_override<Velocity>({})
		.set_override<Acceleration>({})
		.set_override<PointLight>(light)
		.set<ModelOffset>(modelOffset)
		.set<SoundClips>(soundClips)
		.set<LoopingClips>(loopingClips)
		.set_override<FlipInfo>({ true, readCfg->at("PlayerStats").at("flipTime").as<int>(), 0 })
		.set<Haptics>(haptics)
		.set<ModelIndex>({ _modelIndex });
	RegisterPrefab("Player", newPrefab);

	return true;
}

bool MAD::PlayerData::Unload(std::shared_ptr<flecs::world> _game)
{
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity _entity, Player&)
		{
			_entity.destruct(); // destroy this entitiy (happens at frame end)
		});
	_game->defer_end(); // required when removing while iterating!

	UnregisterPrefab("Player");

	return true;
}
