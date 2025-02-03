#include "ModelLoader.h"

MAD::ModelLoader::ModelLoader()
{
	models.clear();
	fbxNames.clear();
}

MAD::ModelLoader::~ModelLoader()
{

}

bool MAD::ModelLoader::InitModels(std::weak_ptr<const GameConfig> _gameConfig, GW::SYSTEM::GLog _log)
{

	_log.LogCategorized("EVENT", "LOADING GAME LEVEL [DATA ORIENTED]");

	//UnloadModels();// clear previous level data if there is any

	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	if (ReadFBXFiles(readCfg->at("AssetPaths").at("modelsPath").as<std::string>().c_str(), _log) == false)
	{
		_log.LogCategorized(
			"ERROR",
			"Fatal error reading FBX data, aborting level load.");

		return false;
	}

	_log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU");

	return true;

}

bool MAD::ModelLoader::FindFBXNames(const char* _fbxFolderPath, GW::SYSTEM::GLog log)
{
	std::string folderPath = _fbxFolderPath;

	WIN32_FIND_DATAA findFileData;

	HANDLE findHandle = FindFirstFileA((folderPath + "*.fbx").c_str(), &findFileData);

	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				fbxNames.push_back(findFileData.cFileName);
			}
		} while (FindNextFileA(findHandle, &findFileData) != 0);

		FindClose(findHandle);

		log.LogCategorized("MESSAGE", "Model Filepaths Found.");
	}
	else
	{
		log.LogCategorized("MESSAGE", "Error opening directory.");
		return false;
	}

	return true;
}

bool MAD::ModelLoader::ReadFBXFiles(const char* _fbxFolderPath, GW::SYSTEM::GLog _log)
{
	_log.LogCategorized("MESSAGE", "Begin Importing .FBX File Data.");
	FindFBXNames(_fbxFolderPath, _log);

	for (int i = 0; i < fbxNames.size(); i += 1)
	{
		Model* model = new Model();
		model->LoadModel(_fbxFolderPath, fbxNames[i]);
		model->vertexStart = vertices.size();
		model->indexStart = indices.size();
		model->materialStart = materials.size();
		models.push_back(*model);
		vertices.insert(vertices.end(), model->vertices.begin(), model->vertices.end());
		indices.insert(indices.end(), model->indices.begin(), model->indices.end());
		materials.insert(materials.end(), model->materials.begin(), model->materials.end());
		delete model;
	}

	return true;
}
