#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "../Systems/MusicLogic.h"
#include "../Systems/PlayerLogic.h"
#include "../Systems/LevelLogic.h"
#include "../Systems/PhysicsLogic.h"
#include "../Systems/CameraLogic.h"
#include "../Systems/UILogic.h"
#include "../Systems/LevelEditorLogic.h"
#include "../Systems/TileLogic.h"
#include "../Systems/AnimationLogic.h"

#include "../Entities/TileData.h"
#include "../Loaders/DelayLoad.h"


namespace MAD
{
	enum GAME_MODES
	{
		RELEASE_MODE = 0,
		//DEBUG_MODE
	};

	class GameLogic
	{
		GW::INPUT::GController gamePads; // controller support
		GW::INPUT::GInput keyboardMouseInput; // twitch keybaord/mouse
		GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse

		GW::AUDIO::GAudio* audioEngine; // can create music & sound effects
		std::shared_ptr<AudioLoader> audioLoader;
		std::shared_ptr<SaveLoader> saveLoader;
		
		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator cameraEventPusher;
		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventGenerator editorEventPusher;
		GW::CORE::GEventGenerator editorModeEventPusher;
		GW::CORE::GEventGenerator editorErrorEventPusher;
		GW::CORE::GEventGenerator animEventPusher;
		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventGenerator touchEventPusher;
		GW::CORE::GEventResponder playEventResponder;
		std::weak_ptr<GameConfig> gameConfig;

		GW::SYSTEM::GDaemon winGamePause{};

		MAD::LevelLogic levelLogic;
		MAD::PhysicsLogic physicsLogic;
		MAD::PlayerLogic playerLogic;
		MAD::CameraLogic cameraLogic;
		MAD::LevelEditorLogic levelEditorLogic;
		MAD::TileLogic tileLogic;
		MAD::MusicLogic musicLogic;
		MAD::AnimationLogic animationLogic;

		TileData* tileData;

		std::shared_ptr<flecs::world> flecsWorld;
		UILogic* ui;
		DirectX11Renderer* renderer;

		flecs::world flecsWorldAsync;
		GW::CORE::GThreadShared flecsWorldLock;

		bool systemsInitialized = false;

		unsigned int splashScreenTime = 4000;
		unsigned int splashScreenStart = -1;

		bool inWinPause;
		unsigned winPauseTime;
	
		bool isPausePressed;
		bool isResetDataPressed;
		bool isBackPressed;
		bool isSubmitPressed;
		bool isCreditsPressed;
		bool isEditorPressed;
		bool isDebugPressed;
		bool isReloadINIPressed;
		unsigned int currState;
		GW::SYSTEM::GWindow window;

	public:

		std::vector<unsigned int> highScores;

		bool Init(std::shared_ptr<flecs::world> _game,
			DirectX11Renderer* _renderer,
			UILogic* _ui, 
			GW::SYSTEM::GWindow _win,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _cameraEventPusher,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			GW::CORE::GEventGenerator _editorEventPusher,
			GW::CORE::GEventGenerator _editorModeEventPusher,
			GW::CORE::GEventGenerator _editorErrorEventPusher,
			GW::CORE::GEventGenerator _levelEventPusher,
			GW::CORE::GEventGenerator _animEventPusher,
			GW::CORE::GEventGenerator _touchEventPusher,
			std::weak_ptr<GameConfig> _gameConfig,
			std::shared_ptr<AudioLoader> _audioLoader,
			std::shared_ptr<SaveLoader> _saveLoader,
			TileData* _tileData);

		void CheckInput();	
		bool Shutdown();

	private:
		void PauseSystems();
		void PlaySystems();
		bool GameplayStart();
		void GameplayStop();
		bool InitInput();
		bool InitEvents();
		void FadeInEvent();
		void SwitchGameState(GAME_STATE _gameState);
		void ToggleDebug();
		void DebugOnEvent();
		void ReloadINIEvent();
	};
};

#endif