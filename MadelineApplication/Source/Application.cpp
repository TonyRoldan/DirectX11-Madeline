#include "Application.h"
#include "unordered_set"

// open some Gateware namespaces for conveinence 
// NEVER do this in a header file!
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
using namespace MAD;

bool Application::Init()
{
	playEventPusher.Create();
	cameraEventPusher.Create();
	gameStateEventPusher.Create();
	editorEventPusher.Create();
	editorModeEventPusher.Create();
	editorErrorEventPusher.Create();
	levelEventPusher.Create();
	animationEventPusher.Create();
	touchEventPusher.Create();

	// load all game settigns
	gameConfig = std::make_shared<GameConfig>();
	// create the ECS system
	flecsWorld = std::make_shared<flecs::world>();
	uiWorld = std::make_shared<flecs::world>();

	audioLoader = std::make_shared<AudioLoader>();
	modelLoader = std::make_shared<ModelLoader>();
	spriteLoader = std::make_shared<SpriteLoader>();
	saveLoader = std::make_shared<SaveLoader>();

	modelLoader->InitModels(gameConfig, log);

	if (InitWindow() == false)
		return false;
	if (InitGraphics() == false)
		return false;
	if (InitAudio(log) == false)
		return false;
	if (InitPrefabs() == false)
		return false;
	if (InitSystems() == false)
		return false;


	return true;
}

bool Application::Run()
{
	bool winClosed = false;
	GW::CORE::GEventResponder winHandler;
	winHandler.Create([&winClosed](GW::GEvent _gEvent)
		{
			GW::SYSTEM::GWindow::Events ev;
			if (+_gEvent.Read(ev) && ev == GW::SYSTEM::GWindow::Events::DESTROY)
				winClosed = true;
		});

	while (+window.ProcessWindowEvents())
	{
		if (winClosed == true)
			return true;

		IDXGISwapChain* swapChain;
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;

		if (+d3d11.GetImmediateContext((void**)&context) &&
			+d3d11.GetRenderTargetView((void**)&targetView) &&
			+d3d11.GetDepthStencilView((void**)&depthStencil) &&
			+d3d11.GetSwapchain((void**)&swapChain))
		{

			gameLogic.CheckInput();
			if (GameLoop() == false)
				return false;
			uiLogic.UpdateUI();
			//d3d11RenderingSystem.UpdateCamera();
			swapChain->Present(1, 0);
			// release incremented COM reference counts
			if (swapChain != nullptr)
				swapChain->Release();
			if (targetView != nullptr)
				targetView->Release();
			if (depthStencil != nullptr)
				depthStencil->Release();
			if (context != nullptr)
				context->Release();

		}
		else
		{
			return false;
		}
	}

	return true;
}

bool Application::Shutdown()
{
	// disconnect systems from global ECS
	d3d11RenderingSystem.Shutdown();
	uiLogic.Shutdown();
	gameLogic.Shutdown();

	modelLoader.reset();
	saveLoader.reset();
	spriteLoader.reset();
	audioLoader.reset();

	return true;
}

bool Application::InitWindow()
{
	// grab settings
	int width = gameConfig->at("Window").at("width").as<int>();
	int height = gameConfig->at("Window").at("height").as<int>();
	int xstart = gameConfig->at("Window").at("xstart").as<int>();
	int ystart = gameConfig->at("Window").at("ystart").as<int>();
	std::string title = gameConfig->at("Window").at("title").as<std::string>();
	// open window
	if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDBORDERED) &&
		+window.SetWindowName(title.c_str()))
	{
		float clr[] = { 0.1f, 0.1f, 0.1f, 1 };
		messages.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clr[2] += 0.01f; // move towards a cyan as they resize
			});
		window.Register(messages);
		return true;
	}
	return false;
}



bool Application::InitGraphics()
{
	if (+d3d11.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		return true;

	return false;
}

bool Application::InitPrefabs()
{
	tileData.Init(flecsWorld, gameConfig);

	unsigned int modelIndex = 0;

	std::string playerModelName = gameConfig->at("Models").at("playerModel").as<std::string>();

	std::unordered_set<std::string> tilesetNames;

	for (int i = 0; gameConfig->at("Tilesets").find("tileset" + std::to_string(i)) != gameConfig->at("Tilesets").end(); i++)
	{
		tilesetNames.insert({ gameConfig->at("Tilesets").at("tileset" + std::to_string(i)).as<std::string>() });
	}

	// Iterate through all the actor models we have in game files.
	for (auto& iter : modelLoader->models)
	{
		std::string modelName(iter.modelName);
		modelName = modelName.substr(0, modelName.size() - 4);
		modelName = TrimDigits(modelName);

		// Tileset model
		if (tilesetNames.find(modelName) != tilesetNames.end())
		{
			tileData.Load(flecsWorld, modelIndex, modelName);
			modelIndex += 1;
		}
		// Player model
		else if (modelName.find(playerModelName) != std::string::npos)
		{
			playerData.Load(flecsWorld, gameConfig, modelIndex, iter, audioLoader.get());
			modelIndex += 1;
		}
	}


	return true;
}

bool Application::InitAudio(GW::SYSTEM::GLog _log)
{
	if (-audioEngine.Create())
		return false;

	audioLoader->Init("../Sounds", "../Music", &audioEngine);
	return true;
}

bool Application::InitSystems()
{
	if (d3d11RenderingSystem.Init(
		window,
		d3d11,
		flecsWorld,
		uiWorld,
		gameConfig,
		modelLoader) == false)
	{
		return false;
	}
	render3DLogic.Init(d3d11,
		&d3d11RenderingSystem,
		flecsWorld,
		window,
		playEventPusher,
		gameStateEventPusher,
		gameConfig,
		modelLoader);

	uiLogic.Init(
		d3d11,
		&d3d11RenderingSystem,
		uiWorld,
		window,
		playEventPusher,
		gameStateEventPusher,
		gameConfig,
		spriteLoader);

	if (gameLogic.Init(
		flecsWorld,
		&d3d11RenderingSystem,
		&uiLogic,
		window,
		playEventPusher,
		cameraEventPusher,
		gameStateEventPusher,
		editorEventPusher,
		editorModeEventPusher,
		editorErrorEventPusher,
		levelEventPusher,
		animationEventPusher,
		touchEventPusher,
		gameConfig,
		audioLoader,
		saveLoader,
		&tileData) == false)
		return false;

	return true;
}

bool Application::GameLoop()
{
	// compute delta time and pass to the ECS system
	static auto startTime = std::chrono::steady_clock::now();
	double elapsedTime = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - startTime).count();
	startTime = std::chrono::steady_clock::now();
	// let the ECS system run
	uiWorld->progress(static_cast<float>(elapsedTime));
	return flecsWorld->progress(static_cast<float>(elapsedTime));
}
