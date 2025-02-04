#ifndef APPLICATION_H
#define APPLICATION_H

// include events
#include "Events/Playevents.h"
// Contains our global game settings
#include "GameConfig.h"

#include "Loaders/AudioLoader.h"
#include "Loaders/ModelLoader.h"
#include "Loaders/SpriteLoader.h"
#include "Loaders/SaveLoader.h"

// Entity prefab data
#include "Entities/PlayerData.h"
#include "Entities/TileData.h"

// Include all systems used by the game and their associated components
#include "Systems/Renderer.h"
#include "Systems/UILogic.h"
#include "Systems/Renderable3DLogic.h"
#include "Systems/ParticleSystem.h"
#include "Systems/GameLogic.h" // must be included after all other systems


// Allocates and runs all sub-systems essential to operating the game
class Application 
{
	// gateware libs used to access operating system
	GW::SYSTEM::GWindow window; // gateware multi-platform window
	
	GW::GRAPHICS::GDirectX11Surface d3d11;
	GW::CORE::GEventResponder messages;
	// third-party gameplay & utility libraries
	std::shared_ptr<flecs::world> flecsWorld; // ECS database for gameplay
	std::shared_ptr<flecs::world> uiWorld;
	std::shared_ptr<GameConfig> gameConfig; // .ini file game settings
	GW::SYSTEM::GLog log;
	// ECS Entities and Prefabs that need to be loaded

	MAD::PlayerData playerData;
	MAD::TileData tileData;

	// specific ECS systems used to run the game
	MAD::DirectX11Renderer d3d11RenderingSystem;
	MAD::GameLogic gameLogic;
	MAD::UILogic uiLogic;
	MAD::Renderable3DLogic render3DLogic;

	// EventGenerator for Game Events
	GW::CORE::GEventGenerator playEventPusher;
	GW::CORE::GEventGenerator cameraEventPusher;
	GW::CORE::GEventGenerator gameStateEventPusher;
	GW::CORE::GEventGenerator editorEventPusher;
	GW::CORE::GEventGenerator editorModeEventPusher;
	GW::CORE::GEventGenerator editorErrorEventPusher;
	GW::CORE::GEventGenerator levelEventPusher;
	GW::CORE::GEventGenerator animationEventPusher;
	GW::CORE::GEventGenerator touchEventPusher;

	GW::AUDIO::GAudio audioEngine; // can create music & sound effects

	std::shared_ptr<MAD::AudioLoader> audioLoader;
	std::shared_ptr<MAD::ModelLoader> modelLoader;
	std::shared_ptr<MAD::SpriteLoader> spriteLoader;
	std::shared_ptr<MAD::SaveLoader> saveLoader;

public:
	bool Init();
	bool Run();
	bool Shutdown();

private:
	bool InitWindow();
	bool InitGraphics();
	bool InitPrefabs();
	bool InitAudio(GW::SYSTEM::GLog _log);
	bool InitSystems();
	bool GameLoop();
};




#endif 