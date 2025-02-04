#ifndef CAMERAEVENTS_H
#define CAMERAEVENTS_H

namespace MAD
{
	enum CameraEvent
	{
		SHAKE
	};

	struct CameraEventData
	{
		GW::MATH::GVECTORF direction;
		float distance;
		float time;
	};
}

#endif