// define all ECS components related to identification
#ifndef IDENTIFICATION_H
#define IDENTIFICATION_H

// example space game (avoid name collisions)
namespace MAD
{
	enum PickupType
	{ 
		STRAWBERRY,
		DASH_CRYSTAL
	};
	struct Sender { unsigned int entityType; };
	struct ControllerID { unsigned index = 0; };

	struct Player {};
	struct Camera {};

	struct Spring {};
	struct Crystal {};
	struct Grave {};
	struct Spikes {};
	struct CrumblingPlatform {};

	struct SceneExit {};

	struct LevelEditorCursor {};
	struct SceneCorner {};
	struct NewSceneCorner {};
};

#endif