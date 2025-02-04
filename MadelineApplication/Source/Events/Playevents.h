#ifndef PLAYEVENTS_H
#define PLAYEVENTS_H

// example space game (avoid name collisions)
namespace MAD
{
	enum PlayEvent
	{
		PLAYER_RESPAWNED,
		KILL_PLAYER,
		PLAYER_DESTROYED,
		GAME_OVER,
		HAPTICS_ACTIVATED,
		TRANSITION_UI,
		LEVEL_RESET,
		DEBUG_ON,
		RELOAD_INI,
		HIT_SPRING,
		HIT_STRAWBERRY,
		COLLECT_STRAWBERRIES,
		HIT_GRAVE,
		HIT_CRYSTAL,
		COLLECT_CRYSTAL,
		HIT_SPIKES
	};

	struct PLAY_EVENT_DATA 
	{
		flecs::id entityId;
		flecs::id otherId;
		unsigned int value;
	};
}

#endif