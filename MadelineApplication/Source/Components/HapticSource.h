#ifndef HAPTIC_SOURCE_H
#define HAPTIC_SOURCE_H

namespace MAD
{
	enum HapticType
	{
		PLAYER_DEATH,
		LAND_GROUND,
		JUMP,
		DASH,
		WALL_CLIMB,
		WALL_SLIDE,
		WALL_SLIDE_NO_STAMINA,
		SPRING_BOUNCE
	};

	struct HapticInfo
	{
		float pan;
		float duration;
		float strength;

		HapticInfo(GW::MATH::GVECTORF info)
		{
			pan = info.x;
			duration = info.y / 1000.0f;
			strength = info.z;
		}
	};

	struct Haptics 
	{
		std::map<HapticType, HapticInfo> info; 
	};
};

#endif