#pragma once
#include "../Systems/Renderer.h"
#include "../Components/Tilemaps.h"
#include "../Components/Tiles.h"


namespace MAD
{
	class Renderable3DLogic
	{
		DirectX11Renderer* renderer;
		std::shared_ptr<flecs::world> game;
		GW::SYSTEM::GWindow window;
		GW::GRAPHICS::GDirectX11Surface d3d;
		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventResponder playEventResponder;
		GW::CORE::GEventResponder gameStateEventResponder;
		GW::CORE::GEventResponder onWindowResize;
		std::weak_ptr<const GameConfig> gameConfig;
		std::shared_ptr<ModelLoader> models;
		

		GAME_STATE currState;

		flecs::query<const ColliderContainer> addColliderQuery;
		flecs::query<const RenderCollider> removeColliderQuery;
		flecs::query<const Moveable> add3DActorsQuery;
		flecs::query<const RenderModel> remove3DModelsQuery;

	public:
		void Init(GW::GRAPHICS::GDirectX11Surface _renderingSurface, 
			DirectX11Renderer* _d3d11RenderingSystem,
			std::shared_ptr<flecs::world> _gameData,
			GW::SYSTEM::GWindow _win,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			std::weak_ptr<const GameConfig> _gameConfig,
			std::shared_ptr<ModelLoader> _models);

		void SetGameState3D(GAME_STATE newState);
		GAME_STATE GetGameState3D();

	private:
		void LoadEvents();
		void ClearColliders();
		void Clear3DTargets();

		float currAspect;
		float newAspect;
		unsigned int screenWidth;
		unsigned int screenHeight;
		unsigned int newHeight;
		unsigned int newWidth;
		float scalar3D;
	};
}
