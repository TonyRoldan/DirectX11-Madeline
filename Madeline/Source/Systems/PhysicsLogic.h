// The level system is responsible for transitioning the various levels in the game
#ifndef PHYSICSLOGIC_H
#define PHYSICSLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"

#include "../Components/Identification.h"
#include "../Components/Gameplay.h"
#include "../Components/Physics.h"
#include "../Components/Tiles.h"

#include "../Events/LevelEvents.h"

// example space game (avoid name collisions)
namespace MAD
{
	class PhysicsLogic
	{
	private:
		std::shared_ptr<flecs::world> flecsWorld;

		std::weak_ptr<const GameConfig> gameConfig;

		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventResponder levelEventHandler;

		flecs::query<Transform, Velocity, Moveable> velocityQuery;
		flecs::query<ColliderContainer, PhysicsCollidable, Collidable> physicsCollidersQuery;
		flecs::query<ColliderContainer, Triggerable, Collidable> triggerCollidersQuery;
		flecs::query<Tile, ColliderContainer> tileColliderQuery;
		
		std::vector<ColliderContainer*> physicsColliders;
		std::vector<ColliderContainer*> triggerColliders;

	public:
		bool Init(	std::shared_ptr<flecs::world> _game, 
					std::weak_ptr<const GameConfig> _gameConfig,
					GW::CORE::GEventGenerator _levelEventPusher);

	private:
		void InitEventHandlers();

		void InitAccelerationSystem();
		void InitTranslationSystem();
		void InitTriggerSystem();

		void HandleTriggerCollisions(
			flecs::entity _entity,
			ColliderContainer& _colliders);

		void HandlePhysicsCollisions(
			flecs::entity _entity,
			const ColliderContainer& _colliders,
			const Transform& _transform,
			Velocity& _velocity);
	public:

		bool Activate(bool _runSystem);

		bool Shutdown();
	};

};

#endif