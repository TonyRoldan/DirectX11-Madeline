#include "TileData.h"
#include "Prefabs.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Tilemaps.h"
#include "../Components/Tiles.h"
#include "../Components/Lights.h"
#include "../Components/Gameplay.h"

std::string MAD::TileData::GetTilePrefabName(unsigned _tilesetId, unsigned _orientationId)
{
	if (tilePrefabNames.at(tilesetNames[_tilesetId]).size() <= _orientationId)
		_orientationId = 0;

	return tilePrefabNames.at(tilesetNames[_tilesetId]).at(_orientationId);
}

unsigned MAD::TileData::GetTileModelIndex(unsigned _tilesetId, unsigned _orientationId)
{
	if (tileModelIndices.at(_tilesetId).size() <= _orientationId)
		_orientationId = 0;

	return tileModelIndices.at(_tilesetId).at(_orientationId);
}

bool MAD::TileData::IsIdOfTileset(unsigned _tilesetId, std::string _tilesetName)
{
	return tilesetNameIndices.at(_tilesetName) == _tilesetId;
}

#pragma region Init
void MAD::TileData::Init(
	std::shared_ptr<flecs::world> _flecsWorld,
	std::weak_ptr<const GameConfig> _gameConfig)
{
	gameConfig = _gameConfig;
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	for (int i = 0; readCfg->at("Tilesets").find("tileset" + std::to_string(i)) != readCfg->at("Tilesets").end(); i++)
	{
		tilesetNames.push_back({ readCfg->at("Tilesets").at("tileset" + std::to_string(i)).as<std::string>() });
		tilesetNameIndices.insert({ tilesetNames[i], tilesetNames.size() -1});
		tileModelIndices.push_back(std::vector<unsigned>());
		tilePrefabNames.insert({ tilesetNames[i], std::vector<std::string>()});
	}
}
#pragma endregion

#pragma region Load
bool MAD::TileData::Load(
	std::shared_ptr<flecs::world> _flecsWorld,
	unsigned int _modelIndex,
	std::string _tilesetName)
{
	// Get tilesetId
	if (tilesetNameIndices.find(_tilesetName) == tilesetNameIndices.end())
		return false;
	unsigned tilsetId = tilesetNameIndices.at(_tilesetName);

	// Prefab orientation specific data
	std::string tileName = _tilesetName + std::to_string(tileModelIndices.at(tilsetId).size());
	tilePrefabNames.at(_tilesetName).push_back(tileName);
	tileModelIndices.at(tilsetId).push_back(_modelIndex);

	// Transform
	Transform transform(_tilesetName, gameConfig);
	if (Equals(transform.value, GW::MATH::GZeroMatrixF))
	{
		transform.value = GIdentityMatrixF;
		GW::MATH::GVECTORF scale = { 50,50,50,1 };
		//scale = MultiplyVector(scale, .5f);
		GW::MATH::GMatrix::ScaleLocalF(transform.value, scale, transform.value);
	}
	ModelOffset modelOffset(_tilesetName, gameConfig);

	// Collider
	ColliderContainer colliders (false, _tilesetName, gameConfig);
	if (colliders.colliders.size() == 0)
		colliders.AddBoxCollider(false, false, GW::MATH::GIdentityVectorF, { 1, 1, 1 });

	// Create prefab
	auto tilePrefab = _flecsWorld->prefab(tileName.c_str())
		.add<PhysicsCollidable>()
		.add<StaticModel>()
		.set_override<Transform>(transform)
		.set_override<ModelOffset>(modelOffset)
		.set_override<ColliderContainer>(colliders)
		.override<Tile>()
		.set<ModelIndex>({ _modelIndex });

	// Light
	if (HasComponent(_tilesetName, "lightPos"))
	{
		tilePrefab.set_override<PointLight>(PointLight(_tilesetName, gameConfig));
	}

	// Tileset specific data
	switch (tilsetId)
	{
	case SPRING_ID: // Spring
		tilePrefab.add<Spring>();
		tilePrefab.add<Triggerable>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case CRYSTAL_ID: // Dash Crystal
		tilePrefab.add<Crystal>();
		tilePrefab.add<Triggerable>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case SPIKES_ID: // Spikes
		tilePrefab.add<Spikes>();
		tilePrefab.add<Triggerable>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case SCENE_EXIT_ID: // Scene exit
		tilePrefab.add<SceneExit>();
		tilePrefab.add<Triggerable>();
		tilePrefab.add<RenderInEditor>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case SPAWNPOINT_ID:
		tilePrefab.add<RenderInEditor>();
		tilePrefab.set<ColliderContainer>({});
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case STRAWBERRY_ID:
		tilePrefab.add<Strawberry>();
		tilePrefab.add<Triggerable>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case GRAVE_ID:
		tilePrefab.add<Grave>();
		tilePrefab.add<Triggerable>();
		tilePrefab.remove<PhysicsCollidable>();
		break;
	case CRUMBLING_PLATFORM_ID:
		tilePrefab.add<CrumblingPlatform>();
		break;
	default:
		break;
	}

	// Register Prefab
	RegisterPrefab(tileName.c_str(), tilePrefab);

	return true;
}

bool MAD::TileData::Unload(std::shared_ptr<flecs::world> _flecsWorld)
{
	return true;
}

bool MAD::TileData::HasComponent(std::string _tilesetName, std::string _componentName)
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();

	if (readCfg->find(_tilesetName.c_str()) != readCfg->end())
	{
		return readCfg->at(_tilesetName.c_str()).find(_componentName.c_str()) != readCfg->at(_tilesetName.c_str()).end();
	}
	return false;
}
#pragma endregion