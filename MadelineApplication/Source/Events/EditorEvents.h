#ifndef EDITOREVENTS_H
#define EDITOREVENTS_H

// example space game (avoid name collisions)
namespace MAD
{
	enum EDITOR_EVENT
	{
		OVERWRITE_TILEMAP,
		ADD_TILE,
		REMOVE_TILE
	};

	struct EDITOR_EVENT_DATA
	{
		USHORT sceneIndex;
		int sceneRow;
		int sceneCol;
		USHORT tileset;
		USHORT orientation;
		unsigned int tileData;
	};

	enum EDITOR_MODE
	{
		EDIT_TILEMAP,
		NEW_SCENE,
		NEW_SPAWNPOINT,
		EDIT_SCENE
	};

	struct EDITOR_MODE_EVENT_DATA
	{
	};


	enum EDITOR_ERROR_EVENT
	{
		NONE,
		NEW_SCENE_COLLISION,
		NEW_SCENE_BAD_DIMENSIONS
	};

	struct EDITOR_ERROR_EVENT_DATA
	{
		int value;
	};
}

#endif