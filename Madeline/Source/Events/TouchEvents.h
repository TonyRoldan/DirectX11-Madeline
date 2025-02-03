#ifndef TOUCHEVENTS_H
#define TOUCHEVENTS_H

enum TouchEvent {
	ENTER_STAND,
	EXIT_STAND,
	ENTER_CLIMB,
	EXIT_CLIMB
};

struct TouchEventData 
{
	flecs::id entityId;
};

#endif
