#ifndef SAVESLOT_H
#define SAVESLOT_H

struct SaveSlot
{
	USHORT sceneIndex;
	USHORT prevSceneIndex;
	USHORT deaths;
	USHORT strawberryCount;
	std::vector<USHORT>strawberries;

	bool IsStrawberryCollected(USHORT _sceneIndex) const
	{
		return std::find(strawberries.begin(), strawberries.end(), _sceneIndex) != strawberries.end();
	}

	static unsigned GetMembersSize()
	{
		return sizeof(USHORT) * 4;
	}
};

#endif