#ifndef UILOGIC_H
#define UILOGIC_H

#include "../Systems/Renderer.h"


namespace MAD
{
	class UILogic
	{
		struct MapModelTex
		{
			unsigned int modelType;
			unsigned int pad[3];
		};

		std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

		DirectX11Renderer* renderer;
		std::shared_ptr<flecs::world> uiWorld;
		flecs::world uiWorldAsync;
		GW::GRAPHICS::GDirectX11Surface d3d;	
		GW::SYSTEM::GWindow window;
		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventResponder playEventResponder;
		GW::CORE::GEventResponder gameStateEventResponder;
		GW::CORE::GEventResponder onWindowResize;
		std::weak_ptr<const GameConfig> gameConfig;
		GW::CORE::GThreadShared uiWorldLock;

		GAME_STATE currState;

		std::unique_ptr<DirectX::SpriteFont> arialBlack;
		std::unique_ptr<DirectX::SpriteFont> verdana;

		flecs::query<const Left, Sprite> leftSpriteResizeQuery;
		flecs::query<const Left, Text> leftTextResizeQuery;

		flecs::query<const Right, Sprite> rightSpriteResizeQuery;
		flecs::query<const Right, Text> rightTextResizeQuery;

		flecs::query<const Center, Sprite> centerSpriteResizeQuery;
		flecs::query<const Center, Text> centerTextResizeQuery;

		flecs::query<const Top, Sprite> topSpriteResizeQuery;
		flecs::query<const Top, Text> topTextResizeQuery;

		flecs::query<const LogoScreen, const Sprite> logoSpriteQuery;
		flecs::query<const DirectXScreen, const Sprite> directSpriteQuery;
		flecs::query<const GatewareScreen, const Sprite> gateSpriteQuery;
		flecs::query<const FlecsScreen, const Sprite> flecsSpriteQuery;
		flecs::query<const TitleScreen, const Sprite> titleSpriteQuery;

		flecs::query<const MainMenu, const Sprite> mainMenuSpriteQuery;
		flecs::query<const MainMenu, const Text> mainMenuTextQuery;

		flecs::query<const PlayGame, const Sprite> playGameSpriteQuery;
		flecs::query<const PlayGame, const Text> playGameTextQuery;

		flecs::query<const PauseGame, const Text> pauseGameTextQuery;

		flecs::query<const GameOverScreen, const Sprite> gameOverSpriteQuery;
		flecs::query<const GameOverScreen, const Text> gameOverTextQuery;

		flecs::query<const Credits, Text> creditsTextQuery;
		flecs::query<const Credits, const RenderText, Text> creditsResizeQuery;

		flecs::query<const RenderSprite> activeSpriteQuery;
		flecs::query<const RenderText> activeTextQuery;

		flecs::query<Sprite> resizeSpriteQuery;
		flecs::query<Text> resizeTextQuery;

		

		GW::MATH::GMATRIXF mapViewMatrix;
		GW::MATH::GVECTORF mapEye;
		GW::MATH::GVECTORF mapAt;
		GW::MATH::GVECTORF mapUp;
		GW::MATH::GMATRIXF mapProjMatrix;
		GW::MATH::GVECTORF mapModelScalar;
		MapModelTex mapModelData;

		unsigned int zoomFactor;

		//----------Sprites----------
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> teamLogoView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> teamLogoTex;
		DirectX::SimpleMath::Vector2 teamLogoPos;
		DirectX::SimpleMath::Vector2 teamLogoOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> directSplashView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> directSplashTex;
		DirectX::SimpleMath::Vector2 directSplashPos;
		DirectX::SimpleMath::Vector2 directSplashOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gateSplashView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> gateSplashTex;
		DirectX::SimpleMath::Vector2 gateSplashPos;
		DirectX::SimpleMath::Vector2 gateSplashOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flecsSplashView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> flecsSplashTex;
		DirectX::SimpleMath::Vector2 flecsSplashPos;
		DirectX::SimpleMath::Vector2 flecsSplashOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> titleView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> titleTex;
		DirectX::SimpleMath::Vector2 titleSpritePos;
		DirectX::SimpleMath::Vector2 titleSpriteOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mainMenuView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> mainMenuTex;
		DirectX::SimpleMath::Vector2 mainMenuSpritePos;
		DirectX::SimpleMath::Vector2 mainMenuSpriteOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mainMenuTitleView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> mainMenuTitleTex;
		DirectX::SimpleMath::Vector2 mainMenuTitleSpritePos;
		DirectX::SimpleMath::Vector2 mainMenuTitleSpriteOrigin;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gameOverView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> gameOverTex;
		DirectX::SimpleMath::Vector2 gameOverPos;
		DirectX::SimpleMath::Vector2 gameOverOrigin;

		//----------Text----------
		DirectX::SimpleMath::Vector2 scoreNumPos;
		DirectX::SimpleMath::Vector2 waveTextPos;
		DirectX::SimpleMath::Vector2 waveNumTextPos;


		Text creditsPropsCopy;
		unsigned int modelType;

		//----------Window Resize----------	
		float originalAspect;
		float currAspect;
		float newAspect;
		unsigned int screenWidth;
		unsigned int screenHeight;
		unsigned int newHeight;
		unsigned int newWidth;

		std::shared_ptr<SpriteLoader> spriteLoader;

		GW::MATH::GMATRIXF* viewMatrix;

		float splashAlpha;
		float creditsOffset;

		float pauseAlpha;
		bool maxReached;
		bool minReached;

		bool fadeInDone;
		float flickerSpeed;
		float deltaTime;
		float scrollSpeed;
		float fadeInSpeed;

	public:
		void Init(GW::GRAPHICS::GDirectX11Surface _renderingSurface,
			DirectX11Renderer* _d3d11RenderingSystem,
			std::shared_ptr<flecs::world> _ui,
			GW::SYSTEM::GWindow _win,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			std::weak_ptr<const GameConfig> _gameConfig, std::shared_ptr<SpriteLoader> _sprites);
		void UpdateStats(unsigned int _lives, unsigned int _score, unsigned int _smartBombs, unsigned int _waves);
		void UpdateLives(unsigned int _lives);
		void UpdateScore(unsigned int _score);
		void UpdateSmartBombs(unsigned int _smartBombs);
		void UpdateWaves(unsigned int _waves);
		void UpdateMiniMap();
		void UpdateUI();
		GAME_STATE GetGameState();
		void SetGameState(GAME_STATE _newState);
		void Resize(unsigned int height, unsigned int width);
		void Shutdown();


	private:
		void ClearRenderTargets();
		void LoadSprites();
		void LoadUIEvents();
		void InitCredits();
		void LoadQueries();
		void LoadFonts();
		void LoadText();
		void NewResize();
		void ResizeGameScreen(float quadWidth, float quadHeight);

		struct CreditsText
		{
			std::wstring text;
		}credits;

	};

}
#endif

//float deltaTime;
//auto currTime = std::chrono::steady_clock::now();
//std::chrono::duration<float> _deltaTime = currTime - prevTime;
//prevTime = currTime;
//deltaTime = _deltaTime.count();