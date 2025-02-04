#pragma once

#include <random>

class Random
{
public:
	static void Init()
	{
		randomEngine.seed(std::random_device()());
	}

	static float RandFloat()
	{
		return (float)distribution(randomEngine) / (float)UINT_MAX;
	}


private:
	static std::mt19937 randomEngine;
	static std::uniform_int_distribution<std::mt19937::result_type> distribution;
};
