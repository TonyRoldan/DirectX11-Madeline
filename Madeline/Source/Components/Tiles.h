#ifndef TILES_H
#define TILES_H

namespace MAD
{
	struct Tile
	{
		USHORT sceneIndex;
		USHORT sceneRow;
		USHORT sceneCol;
	};

	struct Strawberry
	{
		GW::MATH::GVECTORF originalPosition;
	};

	struct Collected {};

	struct FollowPlayer {};

	struct TimeCollected { long long value; };

	struct Touched{};
	struct Crumbled{};
	struct TimeTouched { long long value; };
	struct TimeCrumbled { long long value; };
};

#endif