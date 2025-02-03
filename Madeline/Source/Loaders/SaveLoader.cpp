#include "SaveLoader.h"
#include "../Entities/TileData.h"

using namespace MAD;

#pragma region Init / Deconstructor
bool SaveLoader::Init(std::weak_ptr<GameConfig> _gameConfig)
{
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	saveSlotsFolderPath = readCfg->at("SaveSlots").at("saveSlotsPath").as<std::string>();
	saveSlotFilePath = saveSlotsFolderPath + readCfg->at("SaveSlots").at("saveSlotFileName").as<std::string>();

	if (!LoadSaveSlot())
	{
		std::cout << "No save slot found\n";
		saveSlot =
		{
			0,0,0,0
		};

		SaveSaveSlot();
	}

	sceneFolderPath = readCfg->at("Scenes").at("scenesPath").as<std::string>();

	if (!LoadAllScenes())
	{
		std::cout << "There is no level data yet\n";
	}

	return true;
}
#pragma endregion

#pragma region Save Slot
bool MAD::SaveLoader::LoadSaveSlot()
{
	std::vector<UCHAR> saveSlotData;
	if (!ReadFile(saveSlotFilePath, saveSlotData))
		return false;
	if (saveSlotData.size() == 0)
		return false;

	std::memcpy(&saveSlot, &saveSlotData[0], SaveSlot::GetMembersSize());
	saveSlot.strawberries.resize(saveSlot.strawberryCount);
	if (saveSlot.strawberryCount > 0)
		std::memcpy(
			&saveSlot.strawberries[0], 
			&saveSlotData[SaveSlot::GetMembersSize()], 
			saveSlot.strawberryCount * sizeof(USHORT));

	return true;
}

bool MAD::SaveLoader::SaveSaveSlot()
{
	std::vector<UCHAR> saveSlotData;
	PushToBLOB(saveSlotData, &saveSlot, SaveSlot::GetMembersSize());
	if (saveSlot.strawberryCount > 0)
		PushToBLOB(saveSlotData, &saveSlot.strawberries[0], saveSlot.strawberryCount * sizeof(USHORT));

	return WriteFile(saveSlotFilePath, saveSlotData);
}
#pragma endregion

#pragma region Scene
bool SaveLoader::LoadScene(std::string _sceneName, std::shared_ptr<Tilemap>& _outTilemap)
{
	std::vector<UCHAR> sceneData;
	if (!ReadFile(sceneFolderPath + _sceneName, sceneData))
		return false;
	if (sceneData.size() == 0)
		return false;

	int dataIndex = 0;

	// copy rows and columns
	unsigned tilemapMembersSize = Tilemap::GetMembersSize();
	Tilemap tilemap;
	std::memcpy(&tilemap, &sceneData[dataIndex], tilemapMembersSize);
	dataIndex += tilemapMembersSize;
	_outTilemap = std::make_shared<Tilemap>(tilemap.originX, tilemap.originY, tilemap.rows, tilemap.columns);

	// copy neighbor indices
	UCHAR neighborCount;
	std::memcpy(&neighborCount, &sceneData[dataIndex], sizeof(UCHAR));
	dataIndex += sizeof(UCHAR);
	_outTilemap->neighborScenes.resize(neighborCount);
	for (int i = 0; i < neighborCount; i++)
	{
		std::memcpy(&_outTilemap->neighborScenes[i], &sceneData[dataIndex], sizeof(USHORT));
		dataIndex += sizeof(USHORT);
	}

	// copy all tiles
	int curRow = 0, curCol = 0;

	for (; dataIndex < sceneData.size(); dataIndex += sizeof(CompressedTile))
	{
		CompressedTile compressedTile;
		std::memcpy(&compressedTile, &sceneData[dataIndex], sizeof(CompressedTile));

		if (compressedTile.tilesetId == SPAWNPOINT_ID)
			_outTilemap->AddSpawnpoint(curRow, curCol, compressedTile.orientationId);

		for (int i = 0; i < compressedTile.tileCount; i++)
		{
			_outTilemap->tiles.at(curRow).at(curCol) = TilemapTile(compressedTile.tilesetId, compressedTile.orientationId);

			curCol++;
			if (curCol == _outTilemap->columns)
			{
				curCol = 0;
				curRow++;
				if (curRow == _outTilemap->rows)
					return true;
			}
		}
	}

	return true;
}

bool SaveLoader::SaveScene(USHORT _sceneIndex)
{
	std::shared_ptr<Tilemap> tilemap = scenes.at(_sceneIndex);
	std::vector<UCHAR> sceneData;

	// push rows and column data
	PushToBLOB(sceneData, tilemap.get(), Tilemap::GetMembersSize());

	// push neighbor indices
	UCHAR neighborCount = (UCHAR)tilemap->neighborScenes.size();
	PushToBLOB(sceneData, &neighborCount, sizeof(UCHAR));
	for (int i = 0; i < neighborCount; i++)
		PushToBLOB(sceneData, &tilemap->neighborScenes[i], sizeof(USHORT));

	// push tile data 
	CompressedTile compressedTile;

	for (int row = 0; row < tilemap->rows; row++)
	{
		for (int col = 0; col < tilemap->columns; col++)
		{
			if (!compressedTile.Equals(tilemap->tiles.at(row).at(col)) || compressedTile.tileCount == UCHAR_MAX)
			{
				if (compressedTile.tileCount > 0)
					PushToBLOB(sceneData, &compressedTile, sizeof(CompressedTile));

				compressedTile = CompressedTile(tilemap->tiles.at(row).at(col));
			}

			compressedTile.tileCount++;
		}
	}

	if (compressedTile.tileCount > 0)
		PushToBLOB(sceneData, &compressedTile, sizeof(CompressedTile));

	WriteFile(GetSceneFileName(_sceneIndex), sceneData);

	return true;
}


bool SaveLoader::LoadAllScenes()
{
	std::vector<std::string> sceneNames;
	if (!FindAllSceneNames(sceneNames))
		return false;

	for (int i = 0; i < sceneNames.size(); i++)
	{
		std::shared_ptr<Tilemap> tilemap;
		if (!LoadScene(sceneNames[i], tilemap))
			return false;

		scenes.push_back(tilemap);
	}

	return true;
}
#pragma endregion

#pragma region Private Helpers
bool MAD::SaveLoader::FindAllSceneNames(std::vector<std::string>& _sceneNames)
{
	WIN32_FIND_DATAA findFileData;
	HANDLE findHandle = FindFirstFileA((sceneFolderPath + "*.txt").c_str(), &findFileData);

	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				_sceneNames.push_back(findFileData.cFileName);
			}
		} while (FindNextFileA(findHandle, &findFileData) != 0);

		FindClose(findHandle);
	}
	else
	{
		return false;
	}

	return true;
}

std::string MAD::SaveLoader::GetSceneFileName(int _sceneIndex)
{
	return sceneFolderPath + "Scene" + std::to_string(_sceneIndex) + ".txt";
}

void MAD::SaveLoader::PushToBLOB(std::vector<UCHAR>& _blob, const void* data, unsigned dataSize)
{
	size_t blobSize = _blob.size();
	_blob.resize(blobSize + dataSize);
	std::memcpy(&_blob[blobSize], data, dataSize);
}
#pragma endregion

#pragma region File IO
bool MAD::SaveLoader::WriteFile(std::string _fileName, const std::vector<UCHAR>& _inData)
{
	std::ofstream file(_fileName, std::ios::out | std::ios::binary);

	if (!file)
		return false;

	std::copy(_inData.cbegin(), _inData.cend(), std::ostream_iterator<UCHAR>(file));

	file.close();

	return true;
}

bool MAD::SaveLoader::ReadFile(std::string _fileName, std::vector<UCHAR>& _outData)
{
	_outData.clear();

	std::ifstream file(_fileName, std::ios::binary);

	if (!file)
		return false;

	if (!file.eof() && !file.fail())
	{
		file.seekg(0, std::ios_base::end);
		std::streampos fileSize = file.tellg();
		_outData.resize(fileSize);

		file.seekg(0, std::ios_base::beg);
		file.read((char*)&_outData[0], fileSize);
	}

	return true;
}
#pragma endregion

#pragma region Public Helpers
USHORT MAD::SaveLoader::AddNewScene(std::shared_ptr<Tilemap> _scene)
{
	scenes.push_back(_scene);

	return (USHORT)(scenes.size() - 1);
}

void MAD::SaveLoader::AddSceneNeighbor(USHORT _scene1Index, USHORT _scene2Index)
{
	std::shared_ptr<Tilemap> scene1 = scenes[_scene1Index];
	std::shared_ptr<Tilemap> scene2 = scenes[_scene2Index];

	if (std::find(scene1->neighborScenes.begin(), scene1->neighborScenes.end(), _scene2Index) == scene1->neighborScenes.end())
		scene1->neighborScenes.push_back(_scene2Index);
	if (std::find(scene2->neighborScenes.begin(), scene2->neighborScenes.end(), _scene1Index) == scene2->neighborScenes.end())
		scene2->neighborScenes.push_back(_scene1Index);
}

void MAD::SaveLoader::RemoveSceneNeighbor(USHORT _scene1Index, USHORT _scene2Index)
{
	std::shared_ptr<Tilemap> scene1 = scenes[_scene1Index];
	std::shared_ptr<Tilemap> scene2 = scenes[_scene2Index];

	// if either scene still has a scene exit containing the other, they are not unlinked as neighbors
	for (int row = 0; row < scene1->tiles.size(); row++)
	{
		for (int col = 0; col < scene1->tiles[row].size(); col++)
		{
			if (scene1->tiles.at(row).at(col).tilesetId == SCENE_EXIT_ID &&
				scene1->tiles.at(row).at(col).orientationId == _scene2Index)
				return;
		}
	}
	for (int row = 0; row < scene2->tiles.size(); row++)
	{
		for (int col = 0; col < scene2->tiles[row].size(); col++)
		{
			if (scene2->tiles.at(row).at(col).tilesetId == SCENE_EXIT_ID &&
				scene2->tiles.at(row).at(col).orientationId == _scene1Index)
				return;
		}
	}

	auto scene1Neighbor = std::find(scene1->neighborScenes.begin(), scene1->neighborScenes.end(), _scene2Index);
	auto scene2Neighbor = std::find(scene2->neighborScenes.begin(), scene2->neighborScenes.end(), _scene1Index);
	if (scene1Neighbor != scene1->neighborScenes.end())
		scene1->neighborScenes.erase(scene1Neighbor);
	if (scene2Neighbor != scene2->neighborScenes.end())
		scene2->neighborScenes.erase(scene2Neighbor);
}

void MAD::SaveLoader::EnterScene(USHORT _sceneIndex, USHORT _prevSceneIndex)
{
	saveSlot.prevSceneIndex = _prevSceneIndex;
	saveSlot.sceneIndex = _sceneIndex;
	SaveSaveSlot();
}

void MAD::SaveLoader::PlayerDied()
{
	saveSlot.deaths++;
	SaveSaveSlot();
}

void MAD::SaveLoader::CollectStrawberry(USHORT _sceneIndex)
{
	if (std::find(saveSlot.strawberries.begin(), saveSlot.strawberries.end(), _sceneIndex) == saveSlot.strawberries.end())
	{
		saveSlot.strawberries.push_back(_sceneIndex);
		saveSlot.strawberryCount = saveSlot.strawberries.size();
		SaveSaveSlot();
	}
}

void MAD::SaveLoader::ResetSaveData()
{
	saveSlot = { 0,0,0,0,{} };
	SaveSaveSlot();
}
#pragma endregion

#pragma region Getters / Setters
const std::vector<std::shared_ptr<Tilemap>>& MAD::SaveLoader::GetAllScenes()
{
	return scenes;
}
std::shared_ptr<Tilemap> MAD::SaveLoader::GetScene(USHORT _sceneIndex)
{
	if (scenes.size() <= _sceneIndex)
		return NULL;

	return scenes.at(_sceneIndex);
}

int MAD::SaveLoader::GetSceneAtPoint(GW::MATH::GVECTORF _point)
{
	for (int i = 0; i < scenes.size(); i++)
	{
		if (scenes[i]->IsPointInside(_point))
		{
			return i;
		}
	}

	return -1;
}

void MAD::SaveLoader::GetScenesAroundPoint(GW::MATH::GVECTORF _point, std::vector<USHORT>& _outSceneIndices)
{
	int sceneIndex;
	GW::MATH::GVECTORF adjacentPoint = _point;
	// left
	adjacentPoint.x = _point.x - 1;
	sceneIndex = GetSceneAtPoint(adjacentPoint);
	if (sceneIndex != -1 && std::find(_outSceneIndices.begin(), _outSceneIndices.end(), sceneIndex) == _outSceneIndices.end())
		_outSceneIndices.push_back(sceneIndex);

	// right
	adjacentPoint.x = _point.x + 1;
	sceneIndex = GetSceneAtPoint(adjacentPoint);
	if (sceneIndex != -1 && std::find(_outSceneIndices.begin(), _outSceneIndices.end(), sceneIndex) == _outSceneIndices.end())
		_outSceneIndices.push_back(sceneIndex);

	// up
	adjacentPoint.x = _point.x;
	adjacentPoint.y = _point.y + 1;
	sceneIndex = GetSceneAtPoint(adjacentPoint);
	if (sceneIndex != -1 && std::find(_outSceneIndices.begin(), _outSceneIndices.end(), sceneIndex) == _outSceneIndices.end())
		_outSceneIndices.push_back(sceneIndex);

	// down
	adjacentPoint.y = _point.y - 1;
	sceneIndex = GetSceneAtPoint(adjacentPoint);
	if (sceneIndex != -1 && std::find(_outSceneIndices.begin(), _outSceneIndices.end(), sceneIndex) == _outSceneIndices.end())
		_outSceneIndices.push_back(sceneIndex);
}

bool MAD::SaveLoader::GetClosestTilePosOfScene(
	GW::MATH::GVECTORF _point,
	USHORT _sceneIndex,
	GW::MATH::GVECTORF& _outWorldPos)
{
	int adjacentSceneIndex;
	GW::MATH::GVECTORF adjacentPoint = _point;
	// left
	adjacentPoint.x = _point.x - 1;
	adjacentSceneIndex = GetSceneAtPoint(adjacentPoint);
	if (adjacentSceneIndex == _sceneIndex)
	{
		_outWorldPos = adjacentPoint;
		return true;
	}

	// right
	adjacentPoint.x = _point.x + 1;
	adjacentSceneIndex = GetSceneAtPoint(adjacentPoint);
	if (adjacentSceneIndex == _sceneIndex)
	{
		_outWorldPos = adjacentPoint;
		return true;
	}

	// up
	adjacentPoint.x = _point.x;
	adjacentPoint.y = _point.y + 1;
	adjacentSceneIndex = GetSceneAtPoint(adjacentPoint);
	if (adjacentSceneIndex == _sceneIndex)
	{
		_outWorldPos = adjacentPoint;
		return true;
	}

	// down
	adjacentPoint.y = _point.y - 1;
	adjacentSceneIndex = GetSceneAtPoint(adjacentPoint);
	if (adjacentSceneIndex == _sceneIndex)
	{
		_outWorldPos = adjacentPoint;
		return true;
	}

	return false;
}

const SaveSlot& MAD::SaveLoader::GetSaveSlot()
{
	return saveSlot;
}
#pragma endregion