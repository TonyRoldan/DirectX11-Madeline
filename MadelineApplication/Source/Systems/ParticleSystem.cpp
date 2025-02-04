
#include "ParticleSystem.h"

using namespace MAD;

MAD::ParticleSystem::ParticleSystem()
{
	particlePool.resize(maxParticles);
}

void MAD::ParticleSystem::UpdateParticles(float deltaTime)
{
	for (auto& particle : particlePool)
	{
		if (!particle.isActive)
			continue;
		
		if (particle.durationElapsed <= 0.0f)
		{
			particle.isActive = false;
			continue;
		}

		particle.durationElapsed -= deltaTime;
		GW::MATH::GVector::ScaleF(particle.velocity, deltaTime, particle.velocity);
		GW::MATH::GVector::AddVectorF(particle.velocity, particle.position, particle.position);
		particle.rotation += 0.01f * deltaTime;		
	}
}

void MAD::ParticleSystem::EmitParticles(const ParticleProps& particleProps)
{
	Particle& particle = particlePool[poolNdx];
	particle.isActive = true;
	particle.position = particleProps.position;
	particle.rotation = Random::RandFloat() * 2.0f * PI;

	particle.velocity = particleProps.velocity;
	particle.velocity.x = particleProps.velocityVariance.x * (Random::RandFloat() - 0.5f);
	particle.velocity.y = particleProps.velocityVariance.y * (Random::RandFloat() - 0.5f);
	particle.velocity.z = particleProps.velocityVariance.z * (Random::RandFloat() - 0.5f);

	particle.startColor = particleProps.startColor;
	particle.endColor = particleProps.endColor;

	particle.totalDuration = particleProps.duration;
	particle.durationElapsed = particleProps.duration;
	particle.startSize = particleProps.startSize + particleProps.sizeVariance * (Random::RandFloat() - 0.5f);
	particle.endSize = particleProps.endSize;

	poolNdx = --poolNdx % particlePool.size();
}
