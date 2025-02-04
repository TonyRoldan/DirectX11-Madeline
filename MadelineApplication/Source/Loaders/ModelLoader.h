// This is a sample of how to load a level in a data oriented fashion.
// Feel free to use this code as a base and tweak it for your needs.
// *NEW* The new version of this loader saves blender names.
#pragma once

#include <thread>
#include "DelayLoad.h"
#include "Model.h"

namespace MAD
{
	

	class ModelLoader
	{

	private:		
		bool FindFBXNames(const char* _fbxFolderPath, GW::SYSTEM::GLog log);
		bool ReadFBXFiles(const char* _fbxFolderPath, GW::SYSTEM::GLog _log);
			
	public:
		std::vector<Model> models;
		std::vector<std::string> fbxNames;

		std::vector<JointVertex> vertices;
		std::vector<unsigned> indices;
		std::vector<Mesh> meshes;
		std::vector<Material> materials;

		ModelLoader();
		~ModelLoader();
		bool InitModels(std::weak_ptr<const GameConfig> _gameConfig, GW::SYSTEM::GLog _log);
	};
};
