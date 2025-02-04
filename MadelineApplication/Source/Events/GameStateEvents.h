#ifndef GAMESTATEEVENTS_H
#define GAMESTATEEVENTS_H

// example space game (avoid name collisions)
namespace MAD
{
	enum GAME_STATE
	{
		LOGO_SCREEN,
		DIRECTX_SCREEN,
		GATEWARE_SCREEN,
		FLECS_SCREEN,
		TITLE_SCREEN,
		MAIN_MENU,
		PLAY_GAME,
		PAUSE_GAME,
		GAME_OVER_SCREEN,
		WIN_SCREEN,
		CREDITS,
		LEVEL_EDITOR
	};

	struct GAME_STATE_EVENT_DATA
	{
		unsigned int value;
	};
}

#endif