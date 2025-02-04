
#include "Renderable3DLogic.h"
#include "../Components/UI.h"

using namespace MAD;

void MAD::Renderable3DLogic::Init(GW::GRAPHICS::GDirectX11Surface _renderingSurface, 
									DirectX11Renderer* _d3d11RenderingSystem,
									std::shared_ptr<flecs::world> _gameData, 
									GW::SYSTEM::GWindow _win, 
									GW::CORE::GEventGenerator _playEventPusher,
									GW::CORE::GEventGenerator _gameStateEventPusher,
									std::weak_ptr<const GameConfig> _gameConfig, 
									std::shared_ptr<ModelLoader> _models)
{
	d3d = _renderingSurface;
	renderer = _d3d11RenderingSystem;
	game = _gameData;
	window = _win;
	playEventPusher = _playEventPusher;
	gameStateEventPusher = _gameStateEventPusher;
	gameConfig = _gameConfig;
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	models = _models;
	currState = GAME_STATE::LOGO_SCREEN;

	scalar3D = 0.0f;

	d3d.GetAspectRatio(currAspect);
	newAspect = currAspect;
	screenWidth = readCfg->at("Window").at("width").as<int>();
	screenHeight = readCfg->at("Window").at("height").as<int>();
	newHeight = screenHeight;
	newWidth = screenWidth;

	removeColliderQuery = game->query<const RenderCollider>();
	addColliderQuery = game->query<const ColliderContainer>();
	remove3DModelsQuery = game->query<const RenderModel>();
	add3DActorsQuery = game->query<const Moveable>();

	LoadEvents();

}

void MAD::Renderable3DLogic::LoadEvents()
{
	gameStateEventResponder.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE eventTag;
			GAME_STATE_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				game->defer_begin();

				switch (eventTag)
				{

					case GAME_STATE::LOGO_SCREEN:
					{
						break;
					}
					case GAME_STATE::DIRECTX_SCREEN:
					{
						break;
					}
					case GAME_STATE::GATEWARE_SCREEN:
					{
						break;
					}
					case GAME_STATE::FLECS_SCREEN:
					{
						break;
					}
					case GAME_STATE::TITLE_SCREEN:
					{
						break;
					}
					case GAME_STATE::MAIN_MENU:
					{
						Clear3DTargets();
						ClearColliders();
						break;
					}
					case GAME_STATE::PLAY_GAME:
					{						
						break;
					}
					case GAME_STATE::PAUSE_GAME:
					{
						break;
					}
					case GAME_STATE::LEVEL_EDITOR:
					{						
						break;
					}
					default:
					{
						break;
					}
				}

				game->defer_end();
			}
		});
	gameStateEventPusher.Register(gameStateEventResponder);

	playEventResponder.Create([this](const GW::GEvent& _event)
		{
			PlayEvent eventTag;
			PLAY_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				game->defer_begin();

				switch (eventTag)
				{
					case PlayEvent::DEBUG_ON:
					{
						addColliderQuery.each([this](flecs::entity& ntt, const ColliderContainer&)
							{
								if (ntt.has<Collidable>())
								{
									ntt.add<RenderCollider>();
								}
							});
						break;
					}
					default:
						break;
				}

				game->defer_end();
			}
		});
	playEventPusher.Register(playEventResponder);

	/*onWindowResize.Create([&](const GW::GEvent& event)
		{
			GW::SYSTEM::GWindow::Events resize;
			if (+event.Read(resize) && (resize == GW::SYSTEM::GWindow::Events::RESIZE || resize == GW::SYSTEM::GWindow::Events::MAXIMIZE))
			{
				PipelineHandles handles{};
				d3d.GetImmediateContext((void**)&handles);

				window.GetClientWidth(newWidth);
				window.GetClientHeight(newHeight);
				d3d.GetAspectRatio(newAspect);

				float widthScale = ((float)newWidth / (float)screenWidth);
				float heightScale = ((float)newHeight / (float)screenHeight);

				scalar3D = (newAspect > currAspect) ? heightScale : widthScale;

				float viewportWidth = screenWidth * scalar3D;
				float viewportHeight = screenHeight * scalar3D;

				float barWidth = newWidth - viewportWidth * 0.5f;
				float barHeight = newHeight - viewportHeight * 0.5f;

				D3D11_VIEWPORT viewport;
				viewport.TopLeftX = barWidth;
				viewport.TopLeftY = barHeight;
				viewport.Width = viewportWidth;
				viewport.Height = viewportHeight;
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;

				renderer->SetViewport(viewport);
				renderer->UpdateBorders(barWidth, barHeight, newWidth, newHeight, newWidth, { 0.0f,1.0f,0.0f,1.0f });
				renderer->UpdateProjectionMatrix(viewportWidth / viewportHeight);
				
				handles.context->Release();
			}
		});
	window.Register(onWindowResize);*/
}


void MAD::Renderable3DLogic::SetGameState3D(GAME_STATE _newState)
{
	currState = _newState;
}

GAME_STATE MAD::Renderable3DLogic::GetGameState3D()
{
	return currState;
}

void MAD::Renderable3DLogic::ClearColliders()
{
	removeColliderQuery.each([this](flecs::entity& ntt, const RenderCollider&)
		{
			if (ntt.has<Collidable>())
			{
				ntt.remove<RenderCollider>();
			}

		});
}

void MAD::Renderable3DLogic::Clear3DTargets()
{
	remove3DModelsQuery.each([this](flecs::entity& ntt, const RenderModel&)
		{
			ntt.remove<RenderModel>();

		});
}
