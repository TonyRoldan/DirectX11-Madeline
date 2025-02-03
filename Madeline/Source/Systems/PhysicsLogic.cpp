#include "PhysicsLogic.h"

#include "../Events/PlayEvents.h"

#include "../Components/AudioSource.h"

#include "../Utils/Macros.h"

using namespace flecs;
using namespace MAD;
using namespace GW;
using namespace MATH;
using namespace CORE;


#pragma region Init
bool PhysicsLogic::Init(std::shared_ptr<world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GEventGenerator _levelEventPusher)
{
	flecsWorld = _game;
	gameConfig = _gameConfig;
	levelEventPusher = _levelEventPusher;
	
	velocityQuery = flecsWorld->query<Transform, Velocity, Moveable>();
	physicsCollidersQuery = flecsWorld->query<ColliderContainer, PhysicsCollidable, Collidable>();
	triggerCollidersQuery = flecsWorld->query<ColliderContainer, Triggerable, Collidable>();
	tileColliderQuery = flecsWorld->query<Tile, ColliderContainer>();

	InitEventHandlers();
	InitAccelerationSystem();
	InitTranslationSystem();
	InitTriggerSystem();

	return true;
}
#pragma endregion

#pragma region Event Handlers
void MAD::PhysicsLogic::InitEventHandlers()
{
	levelEventHandler.Create([this](const GW::GEvent& _event)
		{
			LEVEL_EVENT event;
			LEVEL_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			default:
			{
				break;
			}
			}
		});
	levelEventPusher.Register(levelEventHandler);
}
#pragma endregion

#pragma region Systems
void MAD::PhysicsLogic::InitAccelerationSystem()
{
	flecsWorld->system<Velocity, const Acceleration, Moveable>("Acceleration System")
		.each([](entity _entity, Velocity& _velocity, const Acceleration& _acceleration, Moveable&)
			{
				GVECTORF accel;
				GVector::ScaleF(_acceleration.value, _entity.delta_time(), accel);
				GVector::AddVectorF(accel, _velocity.value, _velocity.value);
			});
}

void MAD::PhysicsLogic::InitTranslationSystem()
{
	struct TranslationSystem {};
	flecsWorld->entity("Translation System").add<TranslationSystem>();
	flecsWorld->system<TranslationSystem>().each([this](TranslationSystem& _s)
		{
			physicsColliders.clear();
			physicsCollidersQuery.each([this](entity _entity, ColliderContainer& _colliders, PhysicsCollidable&, Collidable&)
				{
					if (!_entity.has<Collidable>())
						int test = 0;
					physicsColliders.push_back(&_colliders);
				});

			velocityQuery.each([this](entity _entity, Transform& _transform, Velocity& _velocity, Moveable&)
				{
					if (_velocity.value.x == 0 && _velocity.value.y == 0)
						return;


					if (_entity.has<ColliderContainer>())
					{
						ColliderContainer colliders = *_entity.get<ColliderContainer>();
						HandlePhysicsCollisions(_entity, colliders, _transform, _velocity);

						GVECTORF amountToMove;
						GVector::ScaleF(_velocity.value, _entity.delta_time(), amountToMove);
						GVector::AddVectorF(amountToMove, _transform.value.row4, _transform.value.row4);

						colliders.UpdateWorldPosition(_transform.value.row4);
						_entity.set<ColliderContainer>(colliders);
					}
					else
					{
						GVECTORF amountToMove;
						GVector::ScaleF(_velocity.value, _entity.delta_time(), amountToMove);
						GVector::AddVectorF(amountToMove, _transform.value.row4, _transform.value.row4);
					}
				});
		});
}

void MAD::PhysicsLogic::InitTriggerSystem()
{
	flecsWorld->system<ColliderContainer, Triggerable, Collidable>("Trigger System")
		.each([this](entity _entity, ColliderContainer& _colliders, Triggerable&, Collidable&)
			{
				HandleTriggerCollisions(_entity, _colliders);
			});
}
#pragma endregion

#pragma region Handle Collision
void PhysicsLogic::HandleTriggerCollisions(
	flecs::entity _entity,
	ColliderContainer& _colliders)
{
	for (auto otherCols : physicsColliders)
	{
		if (otherCols->ownerId == _colliders.ownerId)
			continue;
		if (!_colliders.InCollisionRange(*otherCols))
		{
			if (_colliders.IsContacting(otherCols))
			{
				_colliders.ExitContacts(otherCols);
				otherCols->ExitContacts(&_colliders);
			}
			continue;
		}
		else if (!_colliders.IsContacting(otherCols))
		{
			_colliders.EnterContacts(otherCols);
			otherCols->EnterContacts(&_colliders);
		}

		for (auto triggerCol : _colliders.triggerColliders)
		{
			for (auto physicsCol : otherCols->physicsColliders)
			{
				if (triggerCol->CollisionCheck(physicsCol.get()) == GCollision::GCollisionCheck::COLLISION)
				{
					if (triggerCol->IsContacting(physicsCol.get()))
						continue;

					triggerCol->EnterContact(physicsCol.get());
				}
				else if (triggerCol->IsContacting(physicsCol.get()))
				{
					triggerCol->ExitContact(physicsCol.get());
				}
			}
		}
	}

}

void PhysicsLogic::HandlePhysicsCollisions(flecs::entity _entity, const ColliderContainer& _colliders, const Transform& _transform, Velocity& _velocity)
{
	std::vector<std::pair<float, const Collider*>> hittableColliders;
	RaycastHit hitResult;

	GVECTORF amountToMove = MultiplyVector(_velocity.value, _entity.delta_time());

	// Fill the vector hittableColliders with all possibly hit colliders
	for (int otherCols = 0; otherCols < physicsColliders.size(); otherCols++)
	{
		if (physicsColliders[otherCols]->ownerId == _colliders.ownerId)
			continue;
		if (!_colliders.InCollisionRange(*physicsColliders[otherCols]))
			continue;

		for (int curCol = 0; curCol < _colliders.physicsColliders.size(); curCol++)
		{
			for (int otherCol = 0; otherCol < physicsColliders[otherCols]->physicsColliders.size(); otherCol++)
			{
				if (_colliders.physicsColliders[curCol]->DynamicCollisionCheck2D(
					physicsColliders[otherCols]->physicsColliders[otherCol].get(),
					amountToMove,
					hitResult) == GCollision::GCollisionCheck::COLLISION)
				{
					hittableColliders.push_back({ hitResult.contactTime, physicsColliders[otherCols]->physicsColliders[otherCol].get() });
				}
			}
		}
	}

	// sort hittable colliders by distance
	std::sort(hittableColliders.begin(), hittableColliders.end(),
		[](const std::pair<float, const Collider*>& a, const std::pair<float, const Collider*>& b)
		{
			return a.first < b.first;
		});

	// apply collisions in order from closest to furthest
	for (int curCol = 0; curCol < _colliders.physicsColliders.size(); curCol++)
	{
		for (int hittableCol = 0; hittableCol < hittableColliders.size(); hittableCol++)
		{
			if (_colliders.physicsColliders[curCol]->DynamicCollisionCheck2D(
				hittableColliders[hittableCol].second,
				amountToMove,
				hitResult) == GCollision::GCollisionCheck::COLLISION)
			{
				GVector::AddVectorF(_velocity.value, MultiplyVector(hitResult.surfaceNormal, AbsVector(_velocity.value)), _velocity.value);
				amountToMove = MultiplyVector(_velocity.value, _entity.delta_time());
			}
		}
	}
}
#pragma endregion

#pragma region Activate / Shutdown
bool PhysicsLogic::Activate(bool _runSystem)
{
	if (_runSystem)
	{
		flecsWorld->entity("Acceleration System").enable();
		flecsWorld->entity("Translation System").enable();
		flecsWorld->entity("Trigger System").enable();
	}
	else
	{
		flecsWorld->entity("Acceleration System").disable();
		flecsWorld->entity("Translation System").disable();
		flecsWorld->entity("Trigger System").disable();
	}

	return true;
}

bool PhysicsLogic::Shutdown()
{
	flecsWorld->entity("Acceleration System").destruct();
	flecsWorld->entity("Translation System").destruct();
	flecsWorld->entity("Trigger System").destruct();

	velocityQuery.destruct();
	physicsCollidersQuery.destruct();
	triggerCollidersQuery.destruct();
	tileColliderQuery.destruct();

	flecsWorld.reset();
	gameConfig.reset();

	return true;
}
#pragma endregion
