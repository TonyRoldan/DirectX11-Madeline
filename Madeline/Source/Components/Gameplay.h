// define all ECS components related to gameplay
#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// example space game (avoid name collisions)
namespace MAD
{

	struct FlipInfo
	{
		bool isFacingRight;
		// how long in milliseconds a flip takes to perform
		int flipTime;
		// how far in degrees the entity has flipped toward the left
		float degreesFlipped;
		// -1 = left, 0 = no flip occuring, 1 = right
		int flipDir;
	};
};

#endif