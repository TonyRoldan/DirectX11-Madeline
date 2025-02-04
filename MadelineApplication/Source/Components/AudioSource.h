#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include "../Loaders/AudioLoader.h"

namespace MAD
{
	struct SoundClips
	{
		std::map<std::string, GW::AUDIO::GSound*> sounds;

#pragma region Constructors
		SoundClips() : sounds({})
		{}

		SoundClips(const SoundClips& _other)
		{
			sounds.clear();
			for (auto sound = _other.sounds.begin(); sound != _other.sounds.end(); sound++)
			{
				sounds.insert({ sound->first, sound->second });
			}
		}

		SoundClips(std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig, AudioLoader* _audioLoader)
		{
			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
			{
				sounds = {};
				return;
			}

			int i = 0;
			std::string name;
			std::string fileName;
			float volume;
			while (readCfg->at(_iniName.c_str()).find("sound" + std::to_string(i) + "Name") != readCfg->at(_iniName.c_str()).end())
			{
				name = readCfg->at(_iniName.c_str()).at("sound" + std::to_string(i) + "Name").as<std::string>();
				fileName = readCfg->at(_iniName.c_str()).at("sound" + std::to_string(i) + "FileName").as<std::string>();
				volume = readCfg->at(_iniName.c_str()).at("sound" + std::to_string(i) + "Volume").as<float>();

				sounds.insert({
						name,
						_audioLoader->CreateSound(fileName, volume)
					});

				i++;
			}
		}
#pragma endregion

		void PlaySound(std::string _name) const
		{
			if (sounds.find(_name) != sounds.end())
				sounds.at(_name)->Play();
		}
	};

	struct LoopingClip {
		GW::AUDIO::GMusic* clip;
		float volume;

		LoopingClip(GW::AUDIO::GMusic* _clip, float _volume) : clip(_clip), volume(_volume)
		{}
	};

	struct LoopingClips {
		std::map<std::string, LoopingClip> sounds;

#pragma region Constructors
		LoopingClips() : sounds({})
		{}

		LoopingClips(const LoopingClips& _other)
		{
			sounds.clear();
			for (auto sound = _other.sounds.begin(); sound != _other.sounds.end(); sound++)
			{
				sounds.insert({ sound->first, sound->second });
			}
		}

		LoopingClips(std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig, AudioLoader* _audioLoader)
		{
			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
			{
				sounds = {};
				return;
			}

			int i = 0;
			std::string name;
			std::string fileName;
			float volume;
			while (readCfg->at(_iniName.c_str()).find("loopSound" + std::to_string(i) + "Name") != readCfg->at(_iniName.c_str()).end())
			{
				name = readCfg->at(_iniName.c_str()).at("loopSound" + std::to_string(i) + "Name").as<std::string>();
				fileName = readCfg->at(_iniName.c_str()).at("loopSound" + std::to_string(i) + "FileName").as<std::string>();
				volume = readCfg->at(_iniName.c_str()).at("loopSound" + std::to_string(i) + "Volume").as<float>();

				sounds.insert({
						name,
						LoopingClip(_audioLoader->CreateSoundLooping(fileName, volume), volume)
					});

				i++;
			}
		}
#pragma endregion

		void PlayLooping(std::string _name) const
		{
			if (sounds.find(_name) != sounds.end())
			{
				sounds.at(_name).clip->SetVolume(sounds.at(_name).volume);
				bool isPlayaing;
				sounds.at(_name).clip->isPlaying(isPlayaing);
				if (!isPlayaing)
					sounds.at(_name).clip->Play(true);
			}
		}
	};

}

#endif
