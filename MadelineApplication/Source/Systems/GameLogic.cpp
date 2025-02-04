#include "GameLogic.h"

bool MAD::GameLogic::Init(
	std::shared_ptr<flecs::world> _game,
	DirectX11Renderer* _renderer,
	UILogic* _ui,
	GW::SYSTEM::GWindow _win,
	GW::CORE::GEventGenerator _playEventPusher,
	GW::CORE::GEventGenerator _cameraEventPusher,
	GW::CORE::GEventGenerator _gameStateEventPusher,
	GW::CORE::GEventGenerator _editorTileEventPusher,
	GW::CORE::GEventGenerator _editorModeEventPusher,
	GW::CORE::GEventGenerator _editorErrorEventPusher,
	GW::CORE::GEventGenerator _levelEventPusher,
	GW::CORE::GEventGenerator _animEventPusher,
	GW::CORE::GEventGenerator _touchEventPusher,
	std::weak_ptr<GameConfig> _gameConfig,
	std::shared_ptr<AudioLoader> _audioLoader,
	std::shared_ptr<SaveLoader> _saveLoader,
	TileData* _tileData)
{
	flecsWorld = _game;
	flecsWorldAsync = flecsWorld->async_stage();
	flecsWorldLock.Create();

	ui = _ui;
	renderer = _renderer;
	window = _win;
	playEventPusher = _playEventPusher;
	cameraEventPusher = _cameraEventPusher;
	gameStateEventPusher = _gameStateEventPusher;
	editorEventPusher = _editorTileEventPusher;
	editorModeEventPusher = _editorModeEventPusher;
	editorErrorEventPusher = _editorErrorEventPusher;
	levelEventPusher = _levelEventPusher;
	animEventPusher = _animEventPusher;
	touchEventPusher = _touchEventPusher;

	gameConfig = _gameConfig;
	audioLoader = _audioLoader;
	saveLoader = _saveLoader;
	saveLoader->Init(_gameConfig);

	tileData = _tileData;

	isPausePressed = false;
	isBackPressed = false;
	isSubmitPressed = false;
	isCreditsPressed = false;

	isDebugPressed = false;

	musicLogic.Init(gameConfig, gameStateEventPusher, audioLoader);

	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	winPauseTime = readCfg->at("GraveStone").at("winPauseTime").as<unsigned>();
	inWinPause = false;

	struct MergeAsyncStages {}; // local definition so we control iteration counts
	flecsWorld->entity("Merge Async Stages").add<MergeAsyncStages>();
	// only happens once per frame at the very start of the frame
	flecsWorld->system<MergeAsyncStages>()
		.kind(flecs::OnLoad) // first defined phase
		.each([this](flecs::entity _entity, MergeAsyncStages& _mergeAsyncChanges)
			{
				// merge any waiting changes from the last frame that happened on other threads
				flecsWorldLock.LockSyncWrite();
				flecsWorldAsync.merge();
				flecsWorldLock.UnlockSyncWrite();
			});

	if (InitInput() == false)
		return false;
	if (InitEvents() == false)
		return false;

	SwitchGameState(GAME_STATE::LOGO_SCREEN);

	return true;
}

bool MAD::GameLogic::InitInput()
{
	if (-gamePads.Create())
		return false;
	if (-keyboardMouseInput.Create(window))
		return false;
	if (-bufferedInput.Create(window))
		return false;
	return true;
}

bool MAD::GameLogic::InitEvents()
{
	playEventResponder.Create([this](const GW::GEvent& _event)
		{
			PlayEvent eventTag;
			PLAY_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				switch (eventTag)
				{
				case PlayEvent::GAME_OVER:
				{
					PauseSystems();
					currState = GAME_STATE::GAME_OVER_SCREEN;
					break;
				}
				case PlayEvent::HIT_GRAVE:
				{
					if (inWinPause)
						break;
					inWinPause = true;
					winGamePause.Create(1, [&]()
						{
							SwitchGameState(GAME_STATE::MAIN_MENU);
							GameplayStop();
							inWinPause = false;

							winGamePause.Pause(0, false);
						}, winPauseTime);
					break;
				}
				default:
					break;
				}
			}
		});
	playEventPusher.Register(playEventResponder);

	return true;
}

void MAD::GameLogic::CheckInput()
{
	// Received input
	float pauseInput = 0, backInput = 0, submitInput = 0, creditsInput = 0, editorInput = 0, debugInput = 0;
	float resetDataInput = 0, reloadINIInput = 0;
	// Final input values including controller and keyboard
	float pauseValue = 0, backValue = 0, submitValue = 0, creditsValue = 0, editorValue = 0, debugValue = 0;
	float resetDataValue = 0, reloadINIValue = 0;

	bool isWindowFocused;
	window.IsFocus(isWindowFocused);

	if (isWindowFocused)
	{
		keyboardMouseInput.GetState(G_KEY_ESCAPE, pauseInput); pauseValue += pauseInput;
		keyboardMouseInput.GetState(G_KEY_DELETE, resetDataInput); resetDataValue += resetDataInput;
		keyboardMouseInput.GetState(G_KEY_X, backInput); backValue += backInput;
		keyboardMouseInput.GetState(G_KEY_C, submitInput); submitValue += submitInput;
		keyboardMouseInput.GetState(G_KEY_Z, creditsInput); creditsValue += creditsInput;
		keyboardMouseInput.GetState(G_KEY_L, editorInput); editorValue += editorInput;
		keyboardMouseInput.GetState(G_KEY_TAB, debugInput); debugValue += debugInput;
		keyboardMouseInput.GetState(G_KEY_F1, reloadINIInput); reloadINIValue += reloadINIInput;
		gamePads.GetState(0, G_START_BTN, pauseInput); pauseValue += pauseInput;
		gamePads.GetState(0, G_SELECT_BTN, resetDataInput); resetDataValue += resetDataInput;
		gamePads.GetState(0, G_EAST_BTN, backInput); backValue += backInput;
		gamePads.GetState(0, G_SOUTH_BTN, submitInput); submitValue += submitInput;
		gamePads.GetState(0, G_NORTH_BTN, creditsInput); creditsValue += creditsInput;
	}

	auto timer = std::chrono::system_clock::now().time_since_epoch();
	unsigned int now = std::chrono::duration_cast<std::chrono::milliseconds>(timer).count();

	switch (currState)
	{
	case GAME_STATE::LOGO_SCREEN:
	{
		if (splashScreenStart == -1)
			splashScreenStart = now;

		if ((submitValue && !isSubmitPressed) || (now - splashScreenStart > splashScreenTime))
		{
			splashScreenStart = now;
			FadeInEvent();
			SwitchGameState(GAME_STATE::DIRECTX_SCREEN);
		}
		break;
	}
	case GAME_STATE::DIRECTX_SCREEN:
	{
		if ((submitValue && !isSubmitPressed) || (now - splashScreenStart > splashScreenTime))
		{
			splashScreenStart = now;
			FadeInEvent();
			SwitchGameState(GAME_STATE::GATEWARE_SCREEN);
		}
		break;
	}
	case GAME_STATE::GATEWARE_SCREEN:
	{
		if ((submitValue && !isSubmitPressed) || (now - splashScreenStart > splashScreenTime))
		{
			splashScreenStart = now;
			FadeInEvent();
			SwitchGameState(GAME_STATE::FLECS_SCREEN);
		}
		break;
	}
	case GAME_STATE::FLECS_SCREEN:
	{
		if ((submitValue && !isSubmitPressed) || (now - splashScreenStart > splashScreenTime))
		{
			splashScreenStart = now;
			FadeInEvent();
			SwitchGameState(GAME_STATE::TITLE_SCREEN);
		}
		break;
	}
	case GAME_STATE::TITLE_SCREEN:
	{
		if ((submitValue && !isSubmitPressed) || (now - splashScreenStart > splashScreenTime))
		{
			FadeInEvent();
			SwitchGameState(GAME_STATE::MAIN_MENU);
		}
		break;
	}
	case GAME_STATE::MAIN_MENU:
	{
		if (submitValue && !isSubmitPressed)
		{
			FadeInEvent();
			GameplayStart();
			SwitchGameState(GAME_STATE::PLAY_GAME);
		}

		if (creditsValue && !isCreditsPressed)
		{
			FadeInEvent();
			SwitchGameState(GAME_STATE::CREDITS);
		}

		if (resetDataValue && !isResetDataPressed)
		{
			saveLoader->ResetSaveData();
		}

		break;
	}
	case GAME_STATE::PLAY_GAME:
	{
		if (pauseValue && !isPausePressed)
		{
			PauseSystems();
			SwitchGameState(GAME_STATE::PAUSE_GAME);
		}
		else if (editorValue && !isEditorPressed)
		{
			SwitchGameState(GAME_STATE::LEVEL_EDITOR);
		}
		else if (debugValue && !isDebugPressed)
		{
			ToggleDebug();
			DebugOnEvent();
		}
		else if (reloadINIValue && !isReloadINIPressed)
		{
			ReloadINIEvent();
		}

		break;
	}
	case GAME_STATE::PAUSE_GAME:
	{
		if ((submitValue && !isSubmitPressed) || (pauseValue && !isPausePressed))
		{
			PlaySystems();
			SwitchGameState(GAME_STATE::PLAY_GAME);
		}
		else if (backValue && !isBackPressed)
		{
			SwitchGameState(GAME_STATE::MAIN_MENU);
			GameplayStop();
		}
		else if (editorValue && !isEditorPressed)
		{
			PlaySystems();
			SwitchGameState(GAME_STATE::LEVEL_EDITOR);
		}

		break;
	}
	case GAME_STATE::GAME_OVER_SCREEN:
	{
		if (submitValue && !isSubmitPressed)
		{
			GameplayStop();
			FadeInEvent();
			GameplayStart();
			SwitchGameState(GAME_STATE::PLAY_GAME);

		}

		if (backValue && !isBackPressed)
		{
			SwitchGameState(GAME_STATE::MAIN_MENU);
			GameplayStop();
		}

		break;
	}
	case GAME_STATE::WIN_SCREEN:
		break;
	case GAME_STATE::CREDITS:
	{
		if (backValue && !isBackPressed)
		{
			FadeInEvent();
			SwitchGameState(GAME_STATE::MAIN_MENU);
		}

		break;
	}
	case GAME_STATE::LEVEL_EDITOR:
	{
		if (editorValue && !isEditorPressed)
		{
			SwitchGameState(GAME_STATE::PLAY_GAME);
		}
		else if (pauseValue && !isPausePressed)
		{
			PauseSystems();
			SwitchGameState(GAME_STATE::PAUSE_GAME);
		}
		else if (debugValue && !isDebugPressed)
		{
			ToggleDebug();
			DebugOnEvent();
		}

		break;
	}

	default:
		break;
	}

	isPausePressed = pauseValue != 0;
	isResetDataPressed = resetDataValue != 0;
	isBackPressed = backValue != 0;
	isSubmitPressed = submitValue != 0;
	isCreditsPressed = creditsValue != 0;
	isEditorPressed = editorValue != 0;
	isDebugPressed = debugValue != 0;
	isReloadINIPressed = reloadINIValue != 0;
}

bool::MAD::GameLogic::GameplayStart()
{
	{
		if (systemsInitialized)
		{
			PlaySystems();
			levelEditorLogic.Reset();
			levelLogic.Reset();
			cameraLogic.Reset();
			return true;
		}

		animationLogic.Init(renderer, flecsWorld, animEventPusher, gameStateEventPusher);

		if (playerLogic.Init(
			flecsWorld,
			window,
			gameConfig,
			keyboardMouseInput,
			gamePads,
			playEventPusher,
			cameraEventPusher,
			levelEventPusher,
			animEventPusher,
			touchEventPusher,
			saveLoader) == false)
			return false;
		if (levelLogic.Init(
			flecsWorld,
			playEventPusher,
			gameStateEventPusher,
			editorEventPusher,
			levelEventPusher,
			gameConfig,
			saveLoader,
			tileData) == false)
			return false;
		if (physicsLogic.Init(flecsWorld, gameConfig, levelEventPusher) == false)
			return false;
		if (tileLogic.Init(
			flecsWorld,
			gameConfig,
			playEventPusher,
			levelEventPusher,
			gameStateEventPusher,
			touchEventPusher,
			saveLoader) == false)
			return false;
		if (cameraLogic.Init(flecsWorld, gameConfig, gameStateEventPusher, cameraEventPusher, saveLoader, renderer) == false)
			return false;
		if (levelEditorLogic.Init(
			window,
			renderer,
			flecsWorld,
			gameConfig,
			keyboardMouseInput,
			gameStateEventPusher,
			editorEventPusher,
			editorModeEventPusher,
			editorErrorEventPusher,
			levelEventPusher,
			saveLoader,
			tileData) == false)
			return false;

		levelLogic.Reset();

		systemsInitialized = true;

		return true;
	}
}

void MAD::GameLogic::GameplayStop()
{
	PauseSystems();

	flecsWorldLock.LockSyncWrite();
	flecsWorldAsync.each([this](flecs::entity _entity, Transform&)
		{
			if (!_entity.has<Camera>())
				_entity.destruct();
		});
	flecsWorldLock.UnlockSyncWrite();

	playerLogic.GameplayStop();
}

void MAD::GameLogic::PauseSystems()
{
	playerLogic.Activate(false);
	levelLogic.Activate(false);
	physicsLogic.Activate(false);
	cameraLogic.Activate(false);
	levelEditorLogic.Activate(false);
	tileLogic.Activate(false);
	animationLogic.Activate(false);
}

void MAD::GameLogic::PlaySystems()
{
	playerLogic.Activate(true);
	levelLogic.Activate(true);
	physicsLogic.Activate(true);
	cameraLogic.Activate(true);
	levelEditorLogic.Activate(true);
	tileLogic.Activate(true);
	animationLogic.Activate(true);
}

void MAD::GameLogic::FadeInEvent()
{
	GW::GEvent stateChanged;
	PLAY_EVENT_DATA data;
	data.value = 0;
	stateChanged.Write(PlayEvent::TRANSITION_UI, data);
	playEventPusher.Push(stateChanged);
}

void MAD::GameLogic::SwitchGameState(GAME_STATE _gameState)
{
	currState = _gameState;

	GW::GEvent stateChanged;
	GAME_STATE_EVENT_DATA data;
	data.value = 0;
	stateChanged.Write(_gameState, data);
	gameStateEventPusher.Push(stateChanged);
}

void MAD::GameLogic::ToggleDebug()
{
	renderer->isDebugOn = !renderer->isDebugOn;
}

void MAD::GameLogic::DebugOnEvent()
{
	GW::GEvent stateChanged;
	PLAY_EVENT_DATA data;
	data.value = 0;
	stateChanged.Write(PlayEvent::DEBUG_ON, data);
	playEventPusher.Push(stateChanged);
}

void MAD::GameLogic::ReloadINIEvent()
{
	std::shared_ptr<GameConfig> writeCfg = gameConfig.lock();
	writeCfg->LoadFromFile();

	GW::GEvent stateChanged;
	PLAY_EVENT_DATA data;
	stateChanged.Write(PlayEvent::RELOAD_INI, data);
	playEventPusher.Push(stateChanged);
}

bool MAD::GameLogic::Shutdown()
{
	if (animationLogic.Shutdown() == false)
		return false;
	if (playerLogic.Shutdown() == false)
		return false;
	if (levelLogic.Shutdown() == false)
		return false;
	if (physicsLogic.Shutdown() == false)
		return false;
	if (cameraLogic.Shutdown() == false)
		return false;
	if (tileLogic.Shutdown() == false)
		return false;
	if (levelEditorLogic.Shutdown() == false)
		return false;

	flecsWorld->entity("Merge Async Stages").destruct();

	flecsWorld.reset();
	gameConfig.reset();
	audioLoader.reset();
	saveLoader.reset();

	return true;
}