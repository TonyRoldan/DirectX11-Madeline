#pragma once
#include "Renderer.h"
#include "../Utils/Random.h"
#include "../Utils/Macros.h"

namespace MAD
{
	struct ParticleProps
	{
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF velocity, velocityVariance;
		GW::MATH::GVECTORF startColor, endColor;
		float startSize, endSize, sizeVariance;
		float duration;
	};

	class ParticleSystem
	{

	public:
		ParticleSystem();

		void UpdateParticles(float deltaTime);
		//void RenderParticles()
		void EmitParticles(const ParticleProps& particleProps);

	private:
		struct Particle
		{
			GW::MATH::GVECTORF position;
			GW::MATH::GVECTORF velocity;
			GW::MATH::GVECTORF startColor, endColor;
			float rotation = 0.0f;
			float startSize, endSize;
			float totalDuration = 1.0f;
			float durationElapsed = 0.0f;
			bool isActive = false;
		};

		std::vector<Particle> particlePool;
		unsigned poolNdx = 999;
		unsigned maxParticles = 1000;
	};
}

