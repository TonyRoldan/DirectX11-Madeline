#ifndef LIGHTS_H
#define LIGHTS_H

namespace MAD
{
	struct PointLight
	{
		GW::MATH::GVECTORF offset;
		GW::MATH::GVECTORF color;
		float radius;

		PointLight() : offset({}), color({}), radius(0) {}

		PointLight(std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig) 
			: offset({}), color({}), radius(0)
		{
			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
			{
				return;
			}

			if (readCfg->at(_iniName.c_str()).find("lightPos") != readCfg->at(_iniName.c_str()).end())
			{
				offset = StringToGVector(readCfg->at(_iniName.c_str()).at("lightPos").as<std::string>());
			}

			if (readCfg->at(_iniName.c_str()).find("lightColor") != readCfg->at(_iniName.c_str()).end())
			{
				color = StringToGVector(readCfg->at(_iniName.c_str()).at("lightColor").as<std::string>());
			}

			if (readCfg->at(_iniName.c_str()).find("lightRadius") != readCfg->at(_iniName.c_str()).end())
			{
				radius = readCfg->at(_iniName.c_str()).at("lightRadius").as<float>();
			}
		}
	};
}

#endif
#pragma once
