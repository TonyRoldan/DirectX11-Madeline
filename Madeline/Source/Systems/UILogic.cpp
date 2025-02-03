#include "UILogic.h"

using namespace MAD;

#pragma region Initialize

void UILogic::Init(GW::GRAPHICS::GDirectX11Surface _renderingSurface,
	DirectX11Renderer* _d3d11RenderingSystem,
	std::shared_ptr<flecs::world> _ui,
	GW::SYSTEM::GWindow _win,
	GW::CORE::GEventGenerator _playEventPusher,
	GW::CORE::GEventGenerator _gameStateEventPusher,
	std::weak_ptr<const GameConfig> _gameConfig, std::shared_ptr<SpriteLoader> _sprites)
{
	d3d = _renderingSurface;
	renderer = _d3d11RenderingSystem;
	uiWorld = _ui;
	uiWorldAsync = uiWorld->async_stage();
	window = _win;
	playEventPusher = _playEventPusher;
	gameStateEventPusher = _gameStateEventPusher;
	uiWorldLock.Create();
	gameConfig = _gameConfig;
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	spriteLoader = _sprites;

	//UI
	d3d.GetAspectRatio(currAspect);
	originalAspect = currAspect;
	newAspect = currAspect;
	screenWidth = readCfg->at("Window").at("width").as<int>();
	screenHeight = readCfg->at("Window").at("height").as<int>();
	creditsOffset = { screenHeight * 2.2f };
	newHeight = screenHeight;
	newWidth = screenWidth;
	currState = GAME_STATE::LOGO_SCREEN;

	maxReached = true;
	minReached = false;
	pauseAlpha = 1.0f;
	splashAlpha = 0.0f;
	fadeInDone = false;
	flickerSpeed = 5.0f;
	scrollSpeed = 60.0f;
	fadeInSpeed = 1.1f;

	struct UIMergeAsyncStages {};
	uiWorld->entity("UIMergeAsyncStages").add<UIMergeAsyncStages>();

	uiWorld->system<UIMergeAsyncStages>()
		.kind(flecs::OnLoad)
		.each([this](flecs::entity _entity, UIMergeAsyncStages& _uiMergeAsync)
			{
				uiWorldLock.LockSyncWrite();
				uiWorldAsync.merge();
				uiWorldLock.UnlockSyncWrite();
			});

	mapModelData = { modelType };

	InitCredits();
	LoadFonts();
	LoadText();
	LoadSprites();
	LoadQueries();
	LoadUIEvents();
}

#pragma endregion

#pragma region Fonts

void MAD::UILogic::LoadFonts()
{
	ID3D11Device* device{};
	d3d.GetDevice((void**)&device);

	arialBlack = std::make_unique<DirectX::SpriteFont>(device, L"../Fonts/ArialBlack.spritefont");
	verdana = std::make_unique<DirectX::SpriteFont>(device, L"../Fonts/Verdana.spritefont");

	device->Release();
}

#pragma endregion

#pragma region Text

void MAD::UILogic::LoadText()
{
	#pragma region Main Menu
	DirectX::SimpleMath::Vector2 menuPlayOrigin = verdana->MeasureString(L"Play [C]");
	DirectX::SimpleMath::Vector2 menuPlaySize = verdana->MeasureString(L"Play [C]");
	menuPlayOrigin.x /= 2;
	menuPlayOrigin.y /= 2;

	TextProperties menuPlayProps;
	menuPlayProps.font.type = verdana.get();
	menuPlayProps.origPos.value.x = { screenWidth * 0.35f };
	menuPlayProps.origPos.value.y = { screenHeight * 0.8f };
	menuPlayProps.newPos.value.x = { screenWidth * 0.35f };
	menuPlayProps.newPos.value.y = { screenHeight * 0.8f };
	menuPlayProps.size.x = menuPlaySize.x;
	menuPlayProps.size.y = menuPlaySize.y;
	menuPlayProps.origScale.value = 1.0f;
	menuPlayProps.newScale.value = 1.0f;
	menuPlayProps.origin.value = menuPlayOrigin;
	menuPlayProps.color.value = DirectX::Colors::Azure;
	menuPlayProps.alpha.value = 1.0f;

	DirectX::SimpleMath::Vector2 deleteOrigin = verdana->MeasureString(L"Delete Save\n   [Delete]");
	DirectX::SimpleMath::Vector2 deleteSize = deleteOrigin;
	deleteOrigin.x /= 2;
	deleteOrigin.y /= 2;

	TextProperties deleteProps;
	deleteProps.font.type = verdana.get();
	deleteProps.origPos.value.x = { screenWidth * 0.1f };
	deleteProps.origPos.value.y = { screenHeight * 0.1f };
	deleteProps.newPos.value.x = { screenWidth * 0.1f };
	deleteProps.newPos.value.y = { screenHeight * 0.1f };
	deleteProps.size.x = deleteSize.x;
	deleteProps.size.y = deleteSize.y;
	deleteProps.origScale.value = 0.65f;
	deleteProps.newScale.value = 0.65f;
	deleteProps.origin.value = deleteOrigin;
	deleteProps.color.value = DirectX::Colors::Azure;
	deleteProps.alpha.value = 1.0f;

	DirectX::SimpleMath::Vector2 menuCreditsOrigin = verdana->MeasureString(L"Credits [Z]");
	DirectX::SimpleMath::Vector2 menuCreditsSize = menuCreditsOrigin;
	menuCreditsOrigin.x /= 2;
	menuCreditsOrigin.y /= 2;

	TextProperties menuCreditsProps;
	menuCreditsProps.font.type = verdana.get();
	menuCreditsProps.origPos.value.x = { screenWidth * 0.65f };
	menuCreditsProps.origPos.value.y = { screenHeight * 0.8f };
	menuCreditsProps.newPos.value.x = { screenWidth * 0.65f };
	menuCreditsProps.newPos.value.y = { screenHeight * 0.8f };
	menuCreditsProps.size.x = menuCreditsSize.x;
	menuCreditsProps.size.y = menuCreditsSize.y;
	menuCreditsProps.origScale.value = 1.0f;
	menuCreditsProps.newScale.value = 1.0f;
	menuCreditsProps.origin.value = menuCreditsOrigin;
	menuCreditsProps.color.value = DirectX::Colors::Azure;
	menuCreditsProps.alpha.value = 1.0f;

	DirectX::SimpleMath::Vector2 menuHSOrigin = verdana->MeasureString(L"High Scores [Tab]");
	menuHSOrigin.x /= 2;
	menuHSOrigin.y /= 2;

	/*TextProperties menuHSProps;
	menuHSProps.font.type = verdana.get();
	menuHSProps.pos.value.x = { screenWidth * 0.5f };
	menuHSProps.pos.value.y = { screenHeight * 0.5f };
	menuHSProps.size.x = menuHSOrigin.x;
	menuHSProps.size.y = menuHSOrigin.y;
	menuHSProps.origScale.value = 0.5f;
	menuHSProps.newScale.value = 0.5f;
	menuHSProps.origin.value = menuHSOrigin;
	menuHSProps.color.value = DirectX::Colors::Azure;
	menuHSProps.alpha.value = 1.0f;*/

	/*DirectX::SimpleMath::Vector2 menuExitOrigin = verdana->MeasureString(L" Exit[Esc]");
	menuExitOrigin.x /= 2;
	menuExitOrigin.y /= 2;

	TextProperties menuExitProps;
	menuExitProps.font.type = verdana.get();
	menuExitProps.pos.value = {};
	menuExitProps.origScale.value = 0.5f;
	menuExitProps.newScale.value = 0.5f;
	menuExitProps.origin.value = menuExitOrigin;
	menuExitProps.color.value = DirectX::Colors::Azure;
	menuExitProps.alpha.value = 1.0f;*/

	#pragma endregion

	#pragma region Pause
	//Pause Game
	DirectX::SimpleMath::Vector2 pauseOrigin = verdana->MeasureString(L"Pause");
	DirectX::SimpleMath::Vector2 pauseSize = pauseOrigin;
	pauseOrigin.x /= 2;
	pauseOrigin.y /= 2;

	TextProperties pauseProps;
	pauseProps.font.type = arialBlack.get();
	pauseProps.origPos.value.x = { screenWidth * 0.5f };
	pauseProps.origPos.value.y = { screenHeight * 0.5f };
	pauseProps.newPos.value.x = { screenWidth * 0.5f };
	pauseProps.newPos.value.y = { screenHeight * 0.5f };
	pauseProps.size.x = pauseSize.x;
	pauseProps.size.y = pauseSize.y;
	pauseProps.origScale.value = 1.5f;
	pauseProps.newScale.value = 1.5f;
	pauseProps.origin.value = pauseOrigin;
	pauseProps.color.value = DirectX::Colors::White;
	pauseProps.alpha.value = 1.0f;

	DirectX::SimpleMath::Vector2 exitOrigin = verdana->MeasureString(L"Exit [X]");
	DirectX::SimpleMath::Vector2 exitSize = exitOrigin;
	exitOrigin.x /= 2;
	exitOrigin.y /= 2;

	TextProperties exitProps;
	exitProps.font.type = verdana.get();
	exitProps.origPos.value.x = { screenWidth * 0.65f };
	exitProps.origPos.value.y = { screenHeight * 0.8f };
	exitProps.newPos.value.x = { screenWidth * 0.65f };
	exitProps.newPos.value.y = { screenHeight * 0.8f };
	exitProps.size.x = exitSize.x;
	exitProps.size.y = exitSize.y;
	exitProps.origScale.value = 0.8f;
	exitProps.newScale.value = 0.8f;
	exitProps.origin.value = exitOrigin;
	exitProps.color.value = DirectX::Colors::White;
	exitProps.alpha.value = 0.8f;

	DirectX::SimpleMath::Vector2 resumeOrigin = verdana->MeasureString(L"Resume [C]");
	DirectX::SimpleMath::Vector2 resumeSize = resumeOrigin;
	resumeOrigin.x /= 2;
	resumeOrigin.y /= 2;

	TextProperties resumeProps;
	resumeProps.font.type = verdana.get();
	resumeProps.origPos.value.x = { screenWidth * 0.35f };
	resumeProps.origPos.value.y = { screenHeight * 0.8f };
	resumeProps.newPos.value.x = { screenWidth * 0.35f };
	resumeProps.newPos.value.y = { screenHeight * 0.8f };
	resumeProps.size.x = resumeSize.x;
	resumeProps.size.y = resumeSize.y;
	resumeProps.origScale.value = 0.8f;
	resumeProps.newScale.value = 0.8f;
	resumeProps.origin.value = resumeOrigin;
	resumeProps.color.value = DirectX::Colors::White;
	resumeProps.alpha.value = 1.0f;

	#pragma endregion

	#pragma region Win Screen

	/*DirectX::SimpleMath::Vector2 retryOrigin = verdana->MeasureString(L"Retry\n[Enter]");
	retryOrigin.x /= 2;
	retryOrigin.y /= 2;

	TextProperties retryProps;
	retryProps.font.type = verdana.get();
	retryProps.pos.value = {};
	retryProps.origScale.value = 0.7f;
	retryProps.newScale.value = 0.7f;
	retryProps.origin.value = retryOrigin;
	retryProps.color.value = DirectX::Colors::White;
	retryProps.alpha.value = 1.0f;

	DirectX::SimpleMath::Vector2 quitOrigin = verdana->MeasureString(L"Quit\n[Esc]");
	quitOrigin.x /= 2;
	quitOrigin.y /= 2;

	TextProperties quitProps;
	quitProps.font.type = verdana.get();
	quitProps.pos.value = {};
	quitProps.origScale.value = 0.7f;
	quitProps.newScale.value = 0.7f;
	quitProps.origin.value = quitOrigin;
	quitProps.color.value = DirectX::Colors::White;
	quitProps.alpha.value = 1.0f;*/

	#pragma endregion

	#pragma region Credits
	//Credits
	DirectX::SimpleMath::Vector2 creditsOrigin = verdana->MeasureString(credits.text.c_str());
	DirectX::SimpleMath::Vector2 creditsSize = creditsOrigin;
	creditsOrigin.x /= 2;
	creditsOrigin.y /= 2;

	TextProperties creditsProps;
	creditsProps.font.type = verdana.get();
	creditsProps.origPos.value = { (screenWidth * 0.5f), creditsOffset };
	creditsProps.newPos.value = { (screenWidth * 0.5f), creditsOffset };
	creditsProps.size.x = creditsSize.x;
	creditsProps.size.y = creditsSize.x;
	creditsProps.origScale.value = 0.325f;
	creditsProps.newScale.value = 0.325f;
	creditsProps.origin.value = creditsOrigin;
	creditsProps.color.value = DirectX::Colors::Azure;
	creditsProps.alpha.value = 1.0f;
	#pragma endregion

	#pragma region Entities
	uiWorldLock.LockSyncWrite();

	uiWorldAsync.entity("MenuPlay")
		.add<MainMenu>()
		.set<Text>({ L"Play [C]", menuPlayProps })
		.add<Left>();

	uiWorldAsync.entity("MenuCredits")
		.add<MainMenu>()
		.set<Text>({ L"Credits [Z]", menuCreditsProps })
		.add<Right>();

	uiWorldAsync.entity("MenuDelete")
		.add<MainMenu>()
		.set<Text>({ L"Delete Save\n   [Delete]", deleteProps })
		.add<Right>();

	uiWorldAsync.entity("Pause")
		.add<PauseGame>()
		.set<Text>({ L"Pause", pauseProps })
		.add<Center>();

	uiWorldAsync.entity("ExitGame")
		.add<PauseGame>()
		.set<Text>({ L"Exit [X]", exitProps })
		.add<Left>();

	uiWorldAsync.entity("ResumeGame")
		.add<PauseGame>()
		.set<Text>({ L"Resume [C]", resumeProps })
		.add<Right>();

	/*uiWorldAsync.entity("QuitGame")
		.add<GameOverScreen>()
		.set<Text>({ L"Quit\n[Esc]", quitProps })
		.add<Left>();

	uiWorldAsync.entity("RetryGame")
		.add<GameOverScreen>()
		.set<Text>({ L"Retry\n[Enter]", retryProps })
		.add<Right>();*/

	uiWorldAsync.entity("Credits")
		.add<Credits>()
		.set<Text>({ credits.text.c_str(), creditsProps });

	uiWorldAsync.merge();
	uiWorldLock.UnlockSyncWrite();
	#pragma endregion
}

#pragma endregion

#pragma region Sprites

void UILogic::LoadSprites()
{
	ID3D11Device* device{};
	d3d.GetDevice((void**)&device);

	#pragma region Logo Screen
	Microsoft::WRL::ComPtr<ID3D11Resource> teamLogoResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/BeesKneesLogo.png", teamLogoResource.GetAddressOf(), teamLogoView.ReleaseAndGetAddressOf());
	teamLogoResource.As(&teamLogoTex);
	CD3D11_TEXTURE2D_DESC teamLogoDesc;
	teamLogoTex->GetDesc(&teamLogoDesc);
	
	teamLogoOrigin.x = float(teamLogoDesc.Width / 2);
	teamLogoOrigin.y = float(teamLogoDesc.Height / 2);
	teamLogoPos.x = screenWidth * 0.5f;
	teamLogoPos.y = screenHeight * 0.5f;

	SpriteProperties teamLogoProps;
	teamLogoProps.size.x = teamLogoDesc.Width;
	teamLogoProps.size.y = teamLogoDesc.Height;
	teamLogoProps.pos.value = teamLogoPos;
	teamLogoProps.origin.value = teamLogoOrigin;
	teamLogoProps.origScale.value = 0.5f;
	teamLogoProps.newScale.value = 0.5f;
	teamLogoProps.spriteCount.numSprites = 1;
	teamLogoProps.alpha.value = 1.0f;
	#pragma endregion

	#pragma region DirectX Screen
	Microsoft::WRL::ComPtr<ID3D11Resource> directSplashResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/DirectX11Logo.png", directSplashResource.GetAddressOf(), directSplashView.ReleaseAndGetAddressOf());
	directSplashResource.As(&directSplashTex);
	CD3D11_TEXTURE2D_DESC directSplashDesc;
	directSplashTex->GetDesc(&directSplashDesc);
	directSplashOrigin.x = float(directSplashDesc.Width / 2);
	directSplashOrigin.y = float(directSplashDesc.Height / 2);
	directSplashPos.x = screenWidth * 0.5f;
	directSplashPos.y = screenHeight * 0.5f;

	SpriteProperties directSplashProps;
	directSplashProps.size.x = directSplashDesc.Width;
	directSplashProps.size.y = directSplashDesc.Height;
	directSplashProps.pos.value = directSplashPos;
	directSplashProps.origin.value = directSplashOrigin;
	directSplashProps.origScale.value = 0.5f;
	directSplashProps.newScale.value = 0.5f;
	directSplashProps.spriteCount.numSprites = 1;
	directSplashProps.alpha.value = 1.0f;
	#pragma endregion 

	#pragma region Gateware Screen
	Microsoft::WRL::ComPtr<ID3D11Resource> gateSplashResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/GatewareLogo.png", gateSplashResource.GetAddressOf(), gateSplashView.ReleaseAndGetAddressOf());
	gateSplashResource.As(&gateSplashTex);
	CD3D11_TEXTURE2D_DESC gateSplashDesc;
	gateSplashTex->GetDesc(&gateSplashDesc);
	gateSplashOrigin.x = float(gateSplashDesc.Width / 2);
	gateSplashOrigin.y = float(gateSplashDesc.Height / 2);
	gateSplashPos.x = screenWidth * 0.5f;
	gateSplashPos.y = screenHeight * 0.5f;

	SpriteProperties gateSplashProps;
	gateSplashProps.size.x = gateSplashDesc.Width;
	gateSplashProps.size.y = gateSplashDesc.Height;
	gateSplashProps.pos.value = gateSplashPos;
	gateSplashProps.origin.value = gateSplashOrigin;
	gateSplashProps.origScale.value = 0.5f;
	gateSplashProps.newScale.value = 0.5f;
	gateSplashProps.spriteCount.numSprites = 1;
	gateSplashProps.alpha.value = 1.0f;

	#pragma endregion

	#pragma region Flecs Screen
	Microsoft::WRL::ComPtr<ID3D11Resource> flecsSplashResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/FlecsLogo.png", flecsSplashResource.GetAddressOf(), flecsSplashView.ReleaseAndGetAddressOf());
	flecsSplashResource.As(&flecsSplashTex);
	CD3D11_TEXTURE2D_DESC flecsSplashDesc;
	flecsSplashTex->GetDesc(&flecsSplashDesc);
	flecsSplashOrigin.x = float(flecsSplashDesc.Width * 0.5f);
	flecsSplashOrigin.y = float(flecsSplashDesc.Height * 0.5f);
	flecsSplashPos.x = screenWidth * 0.5f;
	flecsSplashPos.y = screenHeight * 0.5f;

	SpriteProperties flecsSplashProps;
	flecsSplashProps.size.x = flecsSplashDesc.Width;
	flecsSplashProps.size.y = flecsSplashDesc.Height;
	flecsSplashProps.pos.value = flecsSplashPos;
	flecsSplashProps.origin.value = flecsSplashOrigin;
	flecsSplashProps.origScale.value = 0.5f;
	flecsSplashProps.newScale.value = 0.5f;
	flecsSplashProps.spriteCount.numSprites = 1;
	flecsSplashProps.alpha.value = 1.0f;
	#pragma endregion 

	#pragma region Title Screen
	Microsoft::WRL::ComPtr<ID3D11Resource> titleResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/MadelineTitle.png", titleResource.GetAddressOf(), titleView.ReleaseAndGetAddressOf());
	titleResource.As(&titleTex);
	CD3D11_TEXTURE2D_DESC titleDesc;
	titleTex->GetDesc(&titleDesc);
	titleSpriteOrigin.x = float(titleDesc.Width * 0.5f);
	titleSpriteOrigin.y = float(titleDesc.Height * 0.5f);
	titleSpritePos.x = screenWidth * 0.5f;
	titleSpritePos.y = screenHeight * 0.5f;

	SpriteProperties titleProps;
	titleProps.size.x = titleDesc.Width;
	titleProps.size.y = titleDesc.Height;
	titleProps.pos.value = titleSpritePos;
	titleProps.origin.value = titleSpriteOrigin;
	titleProps.origScale.value = 0.5f;
	titleProps.newScale.value = 0.5f;
	titleProps.spriteCount.numSprites = 1;
	titleProps.alpha.value = 1.0f;

	#pragma endregion

	#pragma region Main Menu

	Microsoft::WRL::ComPtr<ID3D11Resource> mainMenuTitleResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/MainMenuTitle.png", mainMenuTitleResource.GetAddressOf(), mainMenuTitleView.ReleaseAndGetAddressOf());
	mainMenuTitleResource.As(&mainMenuTitleTex);
	CD3D11_TEXTURE2D_DESC mainMenuTitleDesc;
	mainMenuTitleTex->GetDesc(&mainMenuTitleDesc);
	mainMenuTitleSpriteOrigin.x = float(mainMenuTitleDesc.Width * 0.5f);
	mainMenuTitleSpriteOrigin.y = float(mainMenuTitleDesc.Height * 0.5f);
	mainMenuTitleSpritePos.x = screenWidth * 0.5f;
	mainMenuTitleSpritePos.y = screenHeight * 0.5f;

	SpriteProperties mainMenuTitleProps;
	mainMenuTitleProps.size.x = mainMenuTitleDesc.Width;
	mainMenuTitleProps.size.y = mainMenuTitleDesc.Height;
	mainMenuTitleProps.pos.value = mainMenuTitleSpritePos;
	mainMenuTitleProps.origin.value = mainMenuTitleSpriteOrigin;
	mainMenuTitleProps.origScale.value = 0.8f;
	mainMenuTitleProps.newScale.value = 0.8f;
	mainMenuTitleProps.spriteCount.numSprites = 1;
	mainMenuTitleProps.alpha.value = 1.0f;

	Microsoft::WRL::ComPtr<ID3D11Resource> mainMenuResource;
	DirectX::CreateWICTextureFromFile(device, L"../Sprites/MainMenu.jpg", mainMenuResource.GetAddressOf(), mainMenuView.ReleaseAndGetAddressOf());
	mainMenuResource.As(&mainMenuTex);
	CD3D11_TEXTURE2D_DESC mainMenuDesc;
	mainMenuTex->GetDesc(&mainMenuDesc);
	mainMenuSpriteOrigin.x = float(mainMenuDesc.Width * 0.5f);
	mainMenuSpriteOrigin.y = float(mainMenuDesc.Height * 0.5f);
	mainMenuSpritePos.x = screenWidth * 0.5f;
	mainMenuSpritePos.y = screenHeight * 0.5f;

	SpriteProperties mainMenuSpriteProps;
	mainMenuSpriteProps.size.x = mainMenuDesc.Width;
	mainMenuSpriteProps.size.y = mainMenuDesc.Height;
	mainMenuSpriteProps.pos.value = mainMenuSpritePos;
	mainMenuSpriteProps.origin.value = mainMenuSpriteOrigin;
	mainMenuSpriteProps.origScale.value = 0.5f;
	mainMenuSpriteProps.newScale.value = 0.5f;
	mainMenuSpriteProps.spriteCount.numSprites = 1;
	mainMenuSpriteProps.alpha.value = 1.0f;
	#pragma endregion

	#pragma region End Game Screen

	

	#pragma endregion 

	#pragma region Entities
	uiWorldLock.LockSyncWrite();

	uiWorldAsync.entity("TeamLogo")
		.add<LogoScreen>()
		.set<Sprite>({ teamLogoView, teamLogoProps })
		.add<Center>();

	uiWorldAsync.entity("DirectSplash")
		.add<DirectXScreen>()
		.set<Sprite>({ directSplashView, directSplashProps })
		.add<Center>();

	uiWorldAsync.entity("GateSplash")
		.add<GatewareScreen>()
		.set<Sprite>({ gateSplashView, gateSplashProps })
		.add<Center>();

	uiWorldAsync.entity("FlecsSplash")
		.add<FlecsScreen>()
		.set<Sprite>({ flecsSplashView, flecsSplashProps })
		.add<Center>();

	uiWorldAsync.entity("Title")
		.add<TitleScreen>()
		.set<Sprite>({ titleView, titleProps })
		.add<Center>();

	uiWorldAsync.entity("MainMenu")
		.add<MainMenu>()
		.set<Sprite>({ mainMenuView, mainMenuSpriteProps })
		.add<Center>();

	uiWorldAsync.entity("MenuTitle")
		.add<MainMenu>()
		.set<Sprite>({ mainMenuTitleView, mainMenuTitleProps })
		.add<Top>();

	uiWorldAsync.merge();
	uiWorldLock.UnlockSyncWrite();

	#pragma endregion
}

#pragma endregion

#pragma region Queries

void MAD::UILogic::LoadQueries()
{
	uiWorldLock.LockSyncWrite();

	leftSpriteResizeQuery = uiWorld->query<const Left, Sprite>();
	leftTextResizeQuery = uiWorld->query<const Left, Text>();

	rightSpriteResizeQuery = uiWorld->query<const Right, Sprite>();
	rightTextResizeQuery = uiWorld->query<const Right, Text>();

	centerSpriteResizeQuery = uiWorld->query<const Center, Sprite>();
	centerTextResizeQuery = uiWorld->query<const Center, Text>();

	topSpriteResizeQuery = uiWorld->query<const Top, Sprite>();
	topTextResizeQuery = uiWorld->query<const Top, Text>();

	creditsResizeQuery = uiWorld->query<const Credits, const RenderText, Text>();

	logoSpriteQuery = uiWorld->query<const LogoScreen, const Sprite>();
	directSpriteQuery = uiWorld->query<const DirectXScreen, const Sprite>();
	gateSpriteQuery = uiWorld->query<const GatewareScreen, const Sprite>();
	flecsSpriteQuery = uiWorld->query<const FlecsScreen, const Sprite>();
	titleSpriteQuery = uiWorld->query<const TitleScreen, const Sprite>();

	mainMenuSpriteQuery = uiWorld->query<const MainMenu, const Sprite>();
	mainMenuTextQuery = uiWorld->query<const MainMenu, const Text>();

	playGameSpriteQuery = uiWorld->query<const PlayGame, const Sprite>();
	playGameTextQuery = uiWorld->query<const PlayGame, const Text>();

	pauseGameTextQuery = uiWorld->query<const PauseGame, const Text>();

	gameOverSpriteQuery = uiWorld->query<const GameOverScreen, const Sprite>();
	gameOverTextQuery = uiWorld->query<const GameOverScreen, const Text>();

	creditsTextQuery = uiWorld->query<const Credits, Text>();

	activeSpriteQuery = uiWorld->query<const RenderSprite>();
	activeTextQuery = uiWorld->query<const RenderText>();

	resizeSpriteQuery = uiWorld->query<Sprite>();
	resizeTextQuery = uiWorld->query<Text>();

	uiWorldAsync.merge();
	uiWorldLock.UnlockSyncWrite();
}

#pragma endregion

#pragma region Events

void MAD::UILogic::LoadUIEvents()
{
	playEventResponder.Create([this](const GW::GEvent& _event)
		{
			PlayEvent eventTag;
			PLAY_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				
			}
		});
	playEventPusher.Register(playEventResponder);

	gameStateEventResponder.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE eventTag;
			GAME_STATE_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				currState = eventTag;

				uiWorld->defer_begin();

				switch (eventTag)
				{
					case GAME_STATE::LOGO_SCREEN:
					{
						ClearRenderTargets();
						logoSpriteQuery.each([this](flecs::entity& ntt, const LogoScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});

						break;
					}
					case GAME_STATE::DIRECTX_SCREEN:
					{
						ClearRenderTargets();
						directSpriteQuery.each([this](flecs::entity& ntt, const DirectXScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});
						break;
					}
					case GAME_STATE::GATEWARE_SCREEN:
					{
						ClearRenderTargets();
						gateSpriteQuery.each([this](flecs::entity& ntt, const GatewareScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});
						break;
					}
					case GAME_STATE::FLECS_SCREEN:
					{
						ClearRenderTargets();
						flecsSpriteQuery.each([this](flecs::entity& ntt, const FlecsScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});
						break;
					}
					case GAME_STATE::TITLE_SCREEN:
					{
						ClearRenderTargets();
						titleSpriteQuery.each([this](flecs::entity& ntt, const TitleScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});
						break;
					}
					case GAME_STATE::MAIN_MENU:
					{
						ClearRenderTargets();
						mainMenuSpriteQuery.each([this](flecs::entity& ntt, const MainMenu&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});

						mainMenuTextQuery.each([this](flecs::entity& ntt, const MainMenu&, const Text& _txt)
							{
								ntt.add<RenderText>();
							});
						break;
					}
					case GAME_STATE::PLAY_GAME:
					{
						ClearRenderTargets();
						playGameSpriteQuery.each([this](flecs::entity& ntt, const PlayGame&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});

						playGameTextQuery.each([this](flecs::entity& ntt, const PlayGame&, const Text& _txt)
							{
								ntt.add<RenderText>();
							});
						break;
					}
					case GAME_STATE::PAUSE_GAME:
					{
						pauseGameTextQuery.each([this](flecs::entity& ntt, const PauseGame&, const Text& _txt)
							{
								ntt.add<RenderText>();
							});
						break;
					}
					case GAME_STATE::GAME_OVER_SCREEN:
					{
						ClearRenderTargets();
						gameOverSpriteQuery.each([this](flecs::entity& ntt, const GameOverScreen&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});

						gameOverTextQuery.each([this](flecs::entity& ntt, const GameOverScreen&, const Text& _txt)
							{
								ntt.add<RenderText>();
							});

						break;
					}
					case GAME_STATE::CREDITS:
					{	
						ClearRenderTargets();
						creditsTextQuery.each([this](flecs::entity& ntt, const Credits&, Text& _txt)
							{
								ntt.add<RenderText>();
							});
						break;
					}
					case GAME_STATE::LEVEL_EDITOR:
					{
						ClearRenderTargets();
						playGameSpriteQuery.each([this](flecs::entity& ntt, const PlayGame&, const Sprite& _sprite)
							{
								ntt.add<RenderSprite>();
							});

						playGameTextQuery.each([this](flecs::entity& ntt, const PlayGame&, const Text& _txt)
							{
								ntt.add<RenderText>();
							});
						break;
					}
					default:
					{
						break;
					}
				}

				uiWorld->defer_end();
			}
		});
	gameStateEventPusher.Register(gameStateEventResponder);

	onWindowResize.Create([&](const GW::GEvent& event)
		{
			GW::SYSTEM::GWindow::Events resize;
			if (+event.Read(resize) && (resize == GW::SYSTEM::GWindow::Events::RESIZE || resize == GW::SYSTEM::GWindow::Events::MAXIMIZE))
			{
				window.GetClientWidth(newWidth);
				window.GetClientHeight(newHeight);
				d3d.GetAspectRatio(newAspect);

				float widthScale = ((float)newWidth / (float)screenWidth);
				float heightScale = ((float)newHeight / (float)screenHeight);
		
				renderer->uiScalar = (newAspect > currAspect) ? heightScale : widthScale;

				float quadWidth, quadHeight;
				if (newAspect > originalAspect)
				{
					quadHeight = 2.0f;
					quadWidth = quadHeight * originalAspect / newAspect;				
				}
				else
				{
					quadWidth = 2.0f;
					quadHeight = quadWidth * newAspect / originalAspect;
				}

				ResizeGameScreen(quadWidth, quadHeight);
				NewResize();
			}
		});
	window.Register(onWindowResize);
}

#pragma endregion

#pragma region Update

void MAD::UILogic::UpdateUI()
{
	auto currTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> _deltaTime = currTime - prevTime;
	prevTime = currTime;
	deltaTime = _deltaTime.count();

	uiWorld->defer_begin();

	switch(currState)
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
			break;
		}
		case GAME_STATE::PLAY_GAME:
		{
			break;
		}
		case GAME_STATE::PAUSE_GAME:
		{
			if (pauseAlpha >= 1.0f)
			{
				maxReached = true;
				minReached = false;
			}

			if (pauseAlpha <= 0.2f)
			{
				maxReached = false;
				minReached = true;
			}

			if (maxReached)
			{
				pauseAlpha -= (deltaTime * flickerSpeed);
			}

			if (minReached)
			{
				pauseAlpha += (deltaTime * flickerSpeed);
			}
			break;
		}
		case GAME_STATE::CREDITS:
		{
			creditsOffset -= (deltaTime * scrollSpeed);

			creditsTextQuery.each([this](flecs::entity& ntt, const Credits&, Text& _txt)
				{					
					_txt.props.newPos.value.y = creditsOffset;
				});	
			break;
		}
		default:
			break;
	}

	uiWorld->defer_end();

	if (splashAlpha >= 1.0f)
	{
		fadeInDone = true;
	}
	else
	{
		fadeInDone = false;
	}

	if (fadeInDone == false)
	{
		splashAlpha += (deltaTime * fadeInSpeed);
	}
}

void MAD::UILogic::NewResize()
{
	resizeSpriteQuery.each([this](flecs::entity& ntt, Sprite& _sprite)
		{
			_sprite.props.newScale.value = _sprite.props.origScale.value * renderer->uiScalar;

			float newSpriteWidth = _sprite.props.size.x * renderer->uiScalar;
			float newSpriteHeight = _sprite.props.size.y * renderer->uiScalar;			

			float offsetX = (newWidth - newSpriteWidth) * 0.5f;
			float offsetY = (newHeight - newSpriteHeight) * 0.5f;

			_sprite.props.pos.value.x = offsetX + newSpriteWidth * 0.5f;
			_sprite.props.pos.value.y = offsetY + newSpriteHeight * 0.5f;

			_sprite.props.origin.value.x = (_sprite.props.size.x * 0.5f);
			_sprite.props.origin.value.y = (_sprite.props.size.y * 0.5f);
		});

	resizeTextQuery.each([this](flecs::entity& ntt, Text& _txt)
		{
			_txt.props.newScale.value = _txt.props.origScale.value * renderer->uiScalar;

			float newTextWidth = _txt.props.size.x * renderer->uiScalar;
			float newTextHeight = _txt.props.size.y * renderer->uiScalar;

			float offsetX = (newWidth - (screenWidth * renderer->uiScalar)) * 0.5f;
			float offsetY = (newHeight - (screenHeight * renderer->uiScalar)) * 0.5f;

			_txt.props.newPos.value.x = _txt.props.origPos.value.x * renderer->uiScalar + offsetX;
			_txt.props.newPos.value.y = _txt.props.origPos.value.y * renderer->uiScalar + offsetY;

			_txt.props.origin.value.x = (_txt.props.size.x * 0.5f);
			_txt.props.origin.value.y = (_txt.props.size.y * 0.5f);
		});



}

void MAD::UILogic::ResizeGameScreen(float quadWidth, float quadHeight)
{
	renderer->gameScreen.verts[0].pos.x = -quadWidth / 2.0f;
	renderer->gameScreen.verts[0].pos.y = quadHeight / 2.0f;

	renderer->gameScreen.verts[1].pos.x = quadWidth / 2.0f;
	renderer->gameScreen.verts[1].pos.y = quadHeight / 2.0f;

	renderer->gameScreen.verts[2].pos.x = -quadWidth / 2.0f;
	renderer->gameScreen.verts[2].pos.y = -quadHeight / 2.0f;

	renderer->gameScreen.verts[3].pos.x = quadWidth / 2.0f;
	renderer->gameScreen.verts[3].pos.y = -quadHeight / 2.0f;
}

GAME_STATE MAD::UILogic::GetGameState()
{
	return currState;
}

void MAD::UILogic::SetGameState(GAME_STATE _newState)
{
	currState = _newState;
}

void MAD::UILogic::ClearRenderTargets()
{
	activeSpriteQuery.each([this](flecs::entity& ntt, const RenderSprite&)
		{
			ntt.remove<RenderSprite>();

		});

	activeTextQuery.each([this](flecs::entity& ntt, const RenderText&)
		{
			ntt.remove<RenderText>();
		});

}

void MAD::UILogic::UpdateStats(unsigned int _lives, unsigned int _score, unsigned int _smartBombs, unsigned int _waves)
{
	UpdateLives(_lives);
	UpdateScore(_score);
	UpdateSmartBombs(_smartBombs);
	UpdateWaves(_waves);
}

void MAD::UILogic::UpdateLives(unsigned int _lives)
{
	uiWorldLock.LockSyncWrite();
	Sprite currSprite = *uiWorldAsync.entity("Lives").get<Sprite>();
	currSprite.props.spriteCount.numSprites = _lives;
	uiWorldAsync.entity("Lives")
		.set<Sprite>({ currSprite });
	uiWorldLock.UnlockSyncWrite();
}

void MAD::UILogic::UpdateScore(unsigned int _score)
{
	std::wstring newScore = std::to_wstring(_score);
	DirectX::SimpleMath::Vector2 scoreOrigin = verdana->MeasureString(newScore.c_str());

	scoreOrigin.y /= 2;
	Text currText = *uiWorldAsync.entity("Score").get<Text>();
	currText.value = newScore;
	currText.props.origin.value = scoreOrigin;

	uiWorldLock.LockSyncWrite();

	uiWorldAsync.entity("Score")
		.set<Text>({ currText });
	uiWorldLock.UnlockSyncWrite();
}

void MAD::UILogic::UpdateSmartBombs(unsigned int _smartBombs)
{
	uiWorldLock.LockSyncWrite();
	uiWorldAsync.entity("SmartBombs")
		.set<SpriteCount>({ _smartBombs });
	uiWorldLock.UnlockSyncWrite();
}

void MAD::UILogic::UpdateWaves(unsigned int _waves)
{
	std::wstring newWave = std::to_wstring(_waves);
	DirectX::SimpleMath::Vector2 waveOrigin = verdana->MeasureString(newWave.c_str());
	waveOrigin.x -= waveOrigin.x;
	waveOrigin.y /= 2;

	Text currText = *uiWorldAsync.entity("WaveNumber").get<Text>();
	currText.value = newWave;
	currText.props.origin.value = waveOrigin;

	uiWorldLock.LockSyncWrite();
	uiWorldAsync.entity("WaveNumber")
		.set<Text>({ currText });
	uiWorldLock.UnlockSyncWrite();
}

void MAD::UILogic::UpdateMiniMap()
{
	mapViewMatrix = renderer->viewMatrix;
}

#pragma endregion

#pragma region Cleanup

void MAD::UILogic::Shutdown()
{
	leftSpriteResizeQuery.destruct();
	rightSpriteResizeQuery.destruct();
	centerSpriteResizeQuery.destruct();
	topSpriteResizeQuery.destruct();

	leftTextResizeQuery.destruct();
	rightTextResizeQuery.destruct();
	centerTextResizeQuery.destruct();
	topTextResizeQuery.destruct();

	logoSpriteQuery.destruct();
	directSpriteQuery.destruct();
	gateSpriteQuery.destruct();
	flecsSpriteQuery.destruct();
	titleSpriteQuery.destruct();

	mainMenuSpriteQuery.destruct();
	mainMenuTextQuery.destruct();

	playGameSpriteQuery.destruct();
	playGameTextQuery.destruct();
	pauseGameTextQuery.destruct();
}

#pragma endregion

#pragma region Credits

void MAD::UILogic::InitCredits()
{
	std::wstring creditsPart1 =
	{
		LR"(      ----------MADELINE----------
      
      Lead Programmer              Keith Babcock

      Gameplay Programmer          Mason Miller

      Graphics Programmer          Antonio Roldan


      Assets:

           Jumping: Pixabay
           https://pixabay.com/sound-effects/body-falling-to-ground-100474/

           Hit Ground: Taine Dry (FS Music Production Bachelor)
           
           Death: Pixabay
           https://pixabay.com/sound-effects/action-hits-dark-and-deep-braam-impact-184274/
           
           Dash: Pixabay
           https://pixabay.com/sound-effects/wing-flap-heavy-prototype-36710/
           


      )"
	};

	std::wstring creditsPart2 = {
		LR"(Music:

            Main Menu- Colorful_Flowers: Tokyo Music Walker
            https://www.chosic.com/download-audio/45508/
            
            Game Music- Pulses Planet: nojisuma
            https://pixabay.com/music/pulses-planet-180961/
            
            Pause Music- Still / Lofi ChillHop Study Beat: SoulProdMusic
            https://pixabay.com/music/beats-still-lofi-chillhop-study-beat-164548/
            
            Win Music- Study: chillmore
            https://pixabay.com/music/beats-study-110111/
            
            Credits Music- Beats Relaxed Vlog Night Street: Ashot-Danielyan-Composer
            https://pixabay.com/sound-effects/angelical-synth-194316/
            
            Level Editor- Beats Royalty Free Use Lofi Chill Background Music Dreamscape: Liderc
            https://pixabay.com/music/beats-royalty-free-use-lofi-chill-background-music-dreamscape-201679/

2D Assets:

            DirectX11 Logo:
            https://forums.daybreakgames.com/ps2/index.php?threads/directx-11-why-not.162542/
            
            Gateware Logo:
            https://gitlab.com/gateware-development
            
            Flecs Logo:
            https://ajmmertens.medium.com/flecs-3-0-is-out-5ca1ac92e8a4
            
            Team Logo/Title Screen
            AI generated by DALL-E
            
            Madeline Title Text Sprite:
            https://www.textstudio.com/logo/editable-celeste-game-logo-705#google_vignette
            
            Title Screen Background
            https://www.peakpx.com/en/hd-wallpaper-desktop-fxkdy/download/1920x1080

3D Assets:

            Celeste- Jackie: Mixamo.com
            
            Low Poly Diamond: holotnaassets
            https://www.cgtrader.com/free-3d-models/various/various-models/low-poly-diamond-for-blender
            
            Mola Spring: diogo-derli
            https://www.cgtrader.com/free-3d-models/industrial/tool/mola
            
            Strawberry: mishka-winowen
            https://www.turbosquid.com/3d-models/3d-strawberry-1912776
            
            Gravestone: Digitalartistjz
            https://www.cgtrader.com/free-3d-models/exterior/other/halloween-themed-assets-gravestones-free

Special thanks to:
Dan Fernandez
Bradley Leffler
Justin Edwards
      )"
	};

	credits.text = creditsPart1 + creditsPart2;
}

#pragma endregion