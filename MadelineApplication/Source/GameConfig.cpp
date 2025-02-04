#include "GameConfig.h"
#include <filesystem>
using namespace std::chrono_literals;

GameConfig::GameConfig() : ini::IniFile() 
{ 
	if (!LoadFromFile())
	{ 
		std::abort(); 
	}
}

GameConfig::~GameConfig() 
{
	(*this).save("../saved.ini");
}

bool GameConfig::LoadFromFile()
{
	const char* defaults = "../defaults.ini";
	const char* saved = "../saved.ini";
	// if they both exist choose the newest one
	if (std::filesystem::exists(defaults) &&
		std::filesystem::exists(saved)) {
		// Load the newer file
		auto defaultsTime = std::filesystem::last_write_time(defaults);
		auto savedTime = std::filesystem::last_write_time(saved);
		if (defaultsTime > savedTime)
			(*this).load("../defaults.ini");
		else
			(*this).load("../saved.ini"); 

		return true;
	}
	// else if saved isn't created, just load defaults
	else if (std::filesystem::exists(defaults)) {
		(*this).load("../defaults.ini");

		return true;
	}

	return false;
}
