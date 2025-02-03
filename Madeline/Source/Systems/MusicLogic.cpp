#include "MusicLogic.h"

#pragma region Init
bool MAD::MusicLogic::Init(
	std::weak_ptr<const GameConfig> _gameConfig, 
	GW::CORE::GEventGenerator _gameStateEventPusher, 
	std::shared_ptr<AudioLoader> _audioLoader)
{
	gameConfig = _gameConfig;
	gameStateEventPusher = _gameStateEventPusher;
	audioLoader = _audioLoader;

	InitAudio();
	InitEventHandlers();

	return true;
}
#pragma endregion

#pragma region Event Handlers
void MAD::MusicLogic::InitEventHandlers()
{
	gameStateEventHandler.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE event;
			GAME_STATE_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case MAD::LOGO_SCREEN:
			{
				OnLogoScreen();
				break;
			}
			case MAD::MAIN_MENU:
			{
				OnMainMenu();
				break;
			}
			case MAD::PLAY_GAME:
			{
				OnPlayGame();
				break;
			}
			case MAD::PAUSE_GAME:
			{
				OnPauseGame();
				break;
			}
			case MAD::WIN_SCREEN:
			{
				OnWinScreen();
				break;
			}
			case MAD::CREDITS:
			{
				OnCredits();
				break;
			}
			case MAD::LEVEL_EDITOR:
			{
				OnLevelEditor();
				break;
			}
			default:
				break;
			}
		});
	gameStateEventPusher.Register(gameStateEventHandler);
}
#pragma endregion

#pragma region Audio Init
void MAD::MusicLogic::InitAudio()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();

	fadeMusicTime = readCfg->at("Music").at("fadeMusicTime").as<unsigned>();

	std::string mainMenuName = readCfg->at("Music").at("mainMenuName").as<std::string>();
	std::string pauseName = readCfg->at("Music").at("pauseName").as<std::string>();
	std::string gameName = readCfg->at("Music").at("gameName").as<std::string>();
	std::string winName = readCfg->at("Music").at("winName").as<std::string>();
	std::string creditsName = readCfg->at("Music").at("creditsName").as<std::string>();
	std::string levelEditorName = readCfg->at("Music").at("levelEditorName").as<std::string>();
	float mainMenuVolume = readCfg->at("Music").at("mainMenuVolume").as<float>();
	float pauseVolume = readCfg->at("Music").at("pauseVolume").as<float>();
	float gameVolume = readCfg->at("Music").at("gameVolume").as<float>();
	float winVolume = readCfg->at("Music").at("winVolume").as<float>();
	float creditsVolume = readCfg->at("Music").at("creditsVolume").as<float>();
	float levelEditorVolume = readCfg->at("Music").at("levelEditorVolume").as<float>();

	curMusic = NULL;
	nextMusic = NULL;
	mainMenuMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(mainMenuName), mainMenuVolume);
	pauseMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(pauseName), pauseVolume);
	gameMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(gameName), gameVolume);
	winMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(winName), winVolume);
	creditsMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(creditsName), creditsVolume);
	levelEditorMusic = std::make_shared<MusicTrack>(audioLoader->GetMusic(levelEditorName), levelEditorVolume);
}
#pragma endregion

#pragma region Audio Functions
void MAD::MusicLogic::PlayMusic(std::shared_ptr<MusicTrack> _music)
{
	if (_music != curMusic)
	{
		auto now = std::chrono::system_clock::now().time_since_epoch();
		fadeMusicStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

		if (curMusic != nextMusic)
		{
			if (curMusic != NULL)
				curMusic->track->Stop();
			curMusic = nextMusic;
		}

		nextMusic = _music;
		nextMusic->track->Play();

		if (curMusic != NULL)
			curMusic->fadeStartVolume = curMusic->curVolume;
		if (nextMusic != NULL)
			nextMusic->fadeStartVolume = nextMusic->curVolume;

		if (fadeMusic == nullptr)
		{
			fadeMusic.Create(1, [this]() {
				auto curTime = std::chrono::system_clock::now().time_since_epoch();
				long now = std::chrono::duration_cast<std::chrono::milliseconds>(curTime).count();
				bool isDone = false;
				float ratio = (now - fadeMusicStartTime) / (float)fadeMusicTime;
				if (ratio > 1)
				{
					isDone = true;
					ratio = 1;
				}

				if (curMusic != NULL)
				{
					curMusic->curVolume = Lerp(curMusic->fadeStartVolume, 0, ratio);
					curMusic->track->SetVolume(curMusic->curVolume);
				}
				if (nextMusic != NULL)
				{
					nextMusic->curVolume = Lerp(nextMusic->fadeStartVolume, nextMusic->maxVolume, ratio);
					nextMusic->track->SetVolume(nextMusic->curVolume);
				}

				if (isDone)
				{
					if (curMusic != NULL)
						curMusic->track->Stop();
					curMusic = nextMusic;
					fadeMusic.Pause(0, false);
				}
				}, 1);
		}
		else
		{
			fadeMusic.Resume();
		}
	}
}
#pragma endregion

#pragma region Game State Events
void MAD::MusicLogic::OnLogoScreen()
{
	PlayMusic(mainMenuMusic);
}

void MAD::MusicLogic::OnMainMenu()
{
	PlayMusic(mainMenuMusic);
}

void MAD::MusicLogic::OnPlayGame()
{
	PlayMusic(gameMusic);
}

void MAD::MusicLogic::OnPauseGame()
{
	PlayMusic(pauseMusic);
}

void MAD::MusicLogic::OnWinScreen()
{
	PlayMusic(winMusic);
}

void MAD::MusicLogic::OnCredits()
{
	PlayMusic(creditsMusic);
}

void MAD::MusicLogic::OnLevelEditor()
{
	PlayMusic(levelEditorMusic);
}
#pragma endregion

#pragma region Activate / Shutdown
bool MAD::MusicLogic::Activate(bool runSystem)
{
	return true;
}

bool MAD::MusicLogic::Shutdown()
{
	return true;
}
#pragma endregion