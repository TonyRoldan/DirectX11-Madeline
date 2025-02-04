#pragma once
#include "../GameConfig.h"
#include "../Loaders/AudioLoader.h"
#include "../Events/GameStateEvents.h"

namespace MAD
{
	class MusicLogic
	{
		struct MusicTrack
		{
			GW::AUDIO::GMusic* track;
			float maxVolume;
			float curVolume;
			float fadeStartVolume;

			MusicTrack(GW::AUDIO::GMusic* _track, float _maxVolume)
			{
				track = _track;
				maxVolume = _maxVolume;
				curVolume = 0;
				fadeStartVolume = 0;
			}
		};

		std::weak_ptr<const GameConfig> gameConfig;

		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventResponder gameStateEventHandler;

		std::shared_ptr<AudioLoader> audioLoader;

		GW::SYSTEM::GDaemon fadeMusic;

		std::shared_ptr<MusicTrack> curMusic;
		std::shared_ptr<MusicTrack> nextMusic;
		std::shared_ptr<MusicTrack> mainMenuMusic;
		std::shared_ptr<MusicTrack> pauseMusic;
		std::shared_ptr<MusicTrack> gameMusic;
		std::shared_ptr<MusicTrack> winMusic;
		std::shared_ptr<MusicTrack> creditsMusic;
		std::shared_ptr<MusicTrack> levelEditorMusic;

		unsigned fadeMusicTime;
		long fadeMusicStartTime;

	public:
		bool Init(
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			std::shared_ptr<AudioLoader> _audioLoader);

	private:

		void InitEventHandlers();
		void InitAudio();

		void PlayMusic(std::shared_ptr<MusicTrack> _music);

		void OnLogoScreen();
		void OnMainMenu();
		void OnPlayGame();
		void OnPauseGame();
		void OnWinScreen();
		void OnCredits();
		void OnLevelEditor();

	public:
		bool Activate(bool runSystem);
		bool Shutdown();
	};
};