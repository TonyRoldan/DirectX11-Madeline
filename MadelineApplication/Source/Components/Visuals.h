// define all ECS components related to drawing
#ifndef VISUALS_H
#define VISUALS_H

// example space game (avoid name collisions)
namespace MAD
{
	struct ModelIndex { unsigned int id; };
	struct RenderModel{};
	struct StaticModel{};
	struct RenderInEditor{};
	struct AnimateModel{};

	struct Color { GW::MATH2D::GVECTOR3F value; };

	struct ModelOffset 
	{ 
		GW::MATH::GVECTORF value; 
	
		ModelOffset() : value({0,0,0,0}) {}

		ModelOffset(GW::MATH::GVECTORF _value) : value(_value) {}

		ModelOffset(std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig)
		{
			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
			{
				value = {0,0,0,0};
				return;
			}

			if (readCfg->at(_iniName.c_str()).find("modelOffset") != readCfg->at(_iniName.c_str()).end())
			{
				value = StringToGVector(readCfg->at(_iniName.c_str()).at("modelOffset").as<std::string>());
				value.w = 0;
			}
			else
			{
				value = {0,0,0,0};
			}
		}
	};
};

#endif