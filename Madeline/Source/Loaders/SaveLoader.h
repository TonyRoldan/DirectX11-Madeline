// This is a sample of how to load a level in a data oriented fashion.
// Feel free to use this code as a base and tweak it for your needs.
// *NEW* The new version of this loader saves blender names.
#pragma once

#include <map>
#include <filesystem>

#include "../GameConfig.h"
#include "../Components/Tilemaps.h"
#include "../Components/SaveSlot.h"

// This reads .h2b files (which are optimized binary .obj+.mtl files) for actor game objects.
namespace MAD
{
	class SaveLoader
	{
	private:
		std::string saveSlotsFolderPath;
		std::string saveSlotFilePath;
		std::string sceneFolderPath;
		
		std::vector<std::shared_ptr<Tilemap>> scenes;
		std::vector<std::vector<USHORT>> sceneConnections;

		SaveSlot saveSlot;

	public:
		bool Init(std::weak_ptr<GameConfig> _gameConfig);

		bool LoadSaveSlot();
		bool SaveSaveSlot();

		bool LoadScene(std::string _sceneName, std::shared_ptr<Tilemap>& _outTilemap);
		bool SaveScene(USHORT _sceneIndex);
		bool LoadAllScenes();

	private:
		bool FindAllSceneNames(std::vector<std::string>& _sceneNames);
		std::string GetSceneFileName(int _sceneIndex);
		void PushToBLOB(std::vector<UCHAR>& _blob, const void* data, unsigned dataSize);

		bool WriteFile(std::string _fileName, const std::vector<UCHAR>& _inData);
		bool ReadFile(std::string _fileName, std::vector<UCHAR>& _outData);

	public:
		// returns the new scene's index
		USHORT AddNewScene(std::shared_ptr<Tilemap> _scene);
		void AddSceneNeighbor(USHORT _scene1Index, USHORT _scene2Index);
		void RemoveSceneNeighbor(USHORT _scene1Index, USHORT _scene2Index);
		void EnterScene(USHORT _sceneIndex, USHORT _prevSceneIndex);
		void PlayerDied();
		void CollectStrawberry(USHORT _sceneIndex);
		void ResetSaveData();

		const std::vector<std::shared_ptr<Tilemap>>& GetAllScenes();
		std::shared_ptr<Tilemap> GetScene(USHORT _sceneIndex);
		// returns -1 if it doesn't collide, otherwise returns the scene's index
		int GetSceneAtPoint(GW::MATH::GVECTORF _point);
		void GetScenesAroundPoint(GW::MATH::GVECTORF _point, std::vector<USHORT>& _outSceneIndices);
		bool GetClosestTilePosOfScene(
			GW::MATH::GVECTORF _point, 
			USHORT _sceneIndex, 
			GW::MATH::GVECTORF& _outWorldPos);
		const SaveSlot& GetSaveSlot();
	};
}