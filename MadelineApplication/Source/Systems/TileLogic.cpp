#include "TileLogic.h"
#include "../Components/Gameplay.h"

bool MAD::TileLogic::Init(
	std::shared_ptr<flecs::world> _flecsWorld,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::CORE::GEventGenerator _playEventPusher,
	GW::CORE::GEventGenerator _levelEventPusher,
	GW::CORE::GEventGenerator _gameStateEventPusher,
	GW::CORE::GEventGenerator _touchEventPusher,
	std::shared_ptr<SaveLoader> _saveLoader)
{
	flecsWorld = _flecsWorld;
	gameConfig = _gameConfig;
	playEventPusher = _playEventPusher;
	levelEventPusher = _levelEventPusher;
	gameStateEventPusher = _gameStateEventPusher;
	touchEventPusher = _touchEventPusher;
	saveLoader = _saveLoader;

	followingStrawberriesQuery = flecsWorld->query<Strawberry, FollowPlayer>();
	playerQuery = flecsWorld->query<Player, Moveable>();

	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	strawberryFollowDist = readCfg->at("Strawberry").at("followDist").as<float>();
	strawberryFollowSmoothing = readCfg->at("Strawberry").at("followSmoothing").as<float>();
	crystalRespawnTime = readCfg->at("Crystal").at("respawnTime").as<unsigned>();
	crumblingPlatformCrumbleTime = readCfg->at("CrumblingPlatform").at("crumbleTime").as<unsigned>();
	crumblingPlatformRespawnTime = readCfg->at("CrumblingPlatform").at("respawnTime").as<unsigned>();

	InitEventHandlers();
	InitSpringSystem();
	InitCrystalSystems();
	InitSpikeSystem();
	InitStrawberrySystem();
	InitGraveSystem();
	InitSceneExitSystem();
	InitCrumblingPlatformSystems();

	return true;
}

void MAD::TileLogic::InitEventHandlers()
{
	playEventHandler.Create([this](const GW::GEvent& _event)
		{
			PlayEvent event;
			PLAY_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case PlayEvent::PLAYER_DESTROYED:
			{
				OnPlayerDestroyed(data);
				break;
			}
			case PlayEvent::COLLECT_STRAWBERRIES:
			{
				OnCollectStrawberries(data);
				break;
			}
			case PlayEvent::COLLECT_CRYSTAL:
			{
				OnCollectCrystal(data);
				break;
			}
			default:
			{
				break;
			}
			}
		});
	playEventPusher.Register(playEventHandler);

	touchEventHandler.Create([this](const GW::GEvent& _event)
		{
			TouchEvent event;
			TouchEventData data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case ENTER_STAND:
			{
				OnEnterTouch(data);
				break;
			}
			case EXIT_STAND:
			{
				OnExitTouch(data);
				break;
			}
			case ENTER_CLIMB:
			{
				OnEnterTouch(data);
				break;
			}
			case EXIT_CLIMB:
			{
				OnExitTouch(data);
				break;
			}
			default:
				break;
			}
		});
	touchEventPusher.Register(touchEventHandler);
}

#pragma region Systems
void MAD::TileLogic::InitSpringSystem()
{
	springSystem = flecsWorld->system<Spring, ColliderContainer>()
		.each([this](Spring&, const ColliderContainer& _colliderContainer) {
		for (auto collider : _colliderContainer.colliders)
		{
			for (auto contact : collider->contacts)
			{
				if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
				{
					PushPlayEvent(PlayEvent::HIT_SPRING);
				}
			}
		}
			});
}

void MAD::TileLogic::InitCrystalSystems()
{
	crystalCollectSystem = flecsWorld->system<Crystal, ColliderContainer, Collidable>()
		.each([this](flecs::entity _entity, Crystal&, const ColliderContainer& _colliderContainer, Collidable& _collidable) {
		for (auto collider : _colliderContainer.colliders)
		{
			if (_entity.has<Collected>())
				return;

			for (auto contact : collider->contacts)
			{
				if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
				{
					PushPlayEvent(PlayEvent::HIT_CRYSTAL, { _entity });
				}
			}
		}
			});

	crystalRespawnSystem = flecsWorld->system<Crystal, Collected, TimeCollected>()
		.each([this](flecs::entity _entity, Crystal&, Collected&, TimeCollected& _timeCollected)
			{
				auto now = GetNow();
				if (now - _timeCollected.value > crystalRespawnTime)
				{
					_entity.add<RenderModel>();
					_entity.add<Collidable>();
					_entity.remove<Collected>();
				}
			});
}

void MAD::TileLogic::InitSpikeSystem()
{
	spikeSystem = flecsWorld->system<Spikes, ColliderContainer>()
		.each([this](flecs::entity _entity, Spikes&, const ColliderContainer& _colliderContainer) {
		for (auto collider : _colliderContainer.colliders)
		{
			for (auto contact : collider->contacts)
			{
				if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
				{
					PushPlayEvent(PlayEvent::HIT_SPIKES);
				}
			}
		}
			});
}

void MAD::TileLogic::InitStrawberrySystem()
{
	strawberrySystem = flecsWorld->system<Strawberry, Transform, ColliderContainer, Tile>()
		.each([this](flecs::entity _entity, Strawberry&, Transform& _transform, const ColliderContainer& _colliderContainer, const Tile& _tile)
			{
				if (_entity.has<Collected>())
					return;

				for (auto collider : _colliderContainer.colliders)
				{
					for (auto contact : collider->contacts)
					{
						if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
						{
							PLAY_EVENT_DATA eventData{};
							eventData.value = _tile.sceneIndex;
							PushPlayEvent(PlayEvent::HIT_STRAWBERRY, eventData);
							_entity.add<FollowPlayer>();
							_entity.remove<Collidable>();
							auto colliderContainer = *_entity.get<ColliderContainer>();
							colliderContainer.DropAllContacts();
							_entity.set<ColliderContainer>(colliderContainer);
						}
					}
				}

				if (_entity.has<FollowPlayer>() && playerQuery.count() > 0)
				{
					GMATRIXF playerTransform = playerQuery.first().get<Transform>()->value;
					if (Distance2D(playerTransform.row4, _transform.value.row4) > strawberryFollowDist)
					{
						GVector::LerpF(
							_transform.value.row4,
							playerTransform.row4,
							strawberryFollowSmoothing * _entity.delta_time(),
							_transform.value.row4);
					}
				}
			});
}

void MAD::TileLogic::InitGraveSystem()
{
	graveSystem = flecsWorld->system<Grave, ColliderContainer>()
		.each([this](Grave&, const ColliderContainer& _colliderContainer) {
		for (auto collider : _colliderContainer.colliders)
		{
			for (auto contact : collider->contacts)
			{
				if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
				{
					PushPlayEvent(PlayEvent::HIT_GRAVE);
				}
			}
		}
			});
}

void MAD::TileLogic::InitSceneExitSystem()
{
	sceneExitSystem = flecsWorld->system<SceneExit, Tile, ColliderContainer, Collidable>()
		.each([this](flecs::entity _entity, SceneExit&, Tile& _tile, ColliderContainer& _colliderContainer, Collidable&)
			{
				for (auto collider : _colliderContainer.colliders)
				{
					for (auto contact : collider->contacts)
					{
						if (flecsWorld->entity(ecs_get_name(*flecsWorld, contact->ownerId)).has<Player>())
						{
							PushLevelEvent(HIT_SCENE_EXIT, { saveLoader->GetScene(_tile.sceneIndex)->GetTile(_tile)->orientationId, _entity });
						}
					}
				}
			});
}

void MAD::TileLogic::InitCrumblingPlatformSystems()
{
	crumblingPlatformCrumbleSystem = flecsWorld->system<CrumblingPlatform, Touched, TimeTouched>()
		.each([this](flecs::entity _entity, CrumblingPlatform&, Touched&, TimeTouched& _timeTouched)
			{
				auto now = GetNow();
				if (now - _timeTouched.value > crumblingPlatformCrumbleTime)
				{
					_entity.remove<Touched>();
					_entity.remove<RenderModel>();
					_entity.remove<Collidable>();
					auto colliderContainer = *_entity.get<ColliderContainer>();
					colliderContainer.DropAllContacts();
					_entity.set<ColliderContainer>(colliderContainer);
					_entity.add<Crumbled>();
					_entity.set<TimeCrumbled>({ GetNow() });
				}
			});

	crumblingPlatformRespawnSystem = flecsWorld->system<CrumblingPlatform, Crumbled, TimeCrumbled>()
		.each([this](flecs::entity _entity, CrumblingPlatform&, Crumbled&, TimeCrumbled& _timeCrumbled)
			{
				auto now = GetNow();
				if (now - _timeCrumbled.value > crumblingPlatformRespawnTime)
				{
					_entity.add<RenderModel>();
					_entity.add<Collidable>();
					_entity.remove<Crumbled>();
				}
			});
}
#pragma endregion

#pragma region Event Pushers
void MAD::TileLogic::PushPlayEvent(PlayEvent _event, PLAY_EVENT_DATA _data)
{
	GW::GEvent playEvent;
	playEvent.Write(_event, _data);
	playEventPusher.Push(playEvent);
}

void MAD::TileLogic::PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _data)
{
	GW::GEvent levelEvent;
	levelEvent.Write(_event, _data);
	levelEventPusher.Push(levelEvent);
}

void MAD::TileLogic::PushGameStateEvent(GAME_STATE _event, GAME_STATE_EVENT_DATA _data)
{
	GW::GEvent gameStateEvent;
	gameStateEvent.Write(_event, _data);
	gameStateEventPusher.Push(gameStateEvent);
}
#pragma endregion

#pragma region Play Events
void MAD::TileLogic::OnPlayerDestroyed(PLAY_EVENT_DATA _data)
{
	flecsWorld->defer_begin();
	followingStrawberriesQuery.each([this](flecs::entity _entity, Strawberry&, FollowPlayer&)
		{
			_entity.remove<FollowPlayer>();
			_entity.add<Collidable>();
			GMATRIXF transform = _entity.get<Transform>()->value;
			transform.row4 = _entity.get<Strawberry>()->originalPosition;
			_entity.set<Transform>({ transform });
		});
	flecsWorld->defer_end();
}

void MAD::TileLogic::OnCollectStrawberries(PLAY_EVENT_DATA _data)
{
	flecsWorld->defer_begin();
	followingStrawberriesQuery.each([this](flecs::entity _entity, Strawberry&, FollowPlayer&)
		{
			saveLoader->CollectStrawberry(_entity.get<Tile>()->sceneIndex);
			_entity.remove<FollowPlayer>();
			_entity.add<Collected>();
			_entity.remove<RenderModel>();
		});
	flecsWorld->defer_end();
}

void MAD::TileLogic::OnCollectCrystal(PLAY_EVENT_DATA _data)
{
	flecsWorld->defer_begin();

	flecs::entity _entity = flecsWorld->entity(ecs_get_name(*flecsWorld, _data.entityId));
	_entity.remove<RenderModel>();
	_entity.remove<Collidable>();
	_entity.add<Collected>();
	_entity.set<TimeCollected>({ GetNow() });
	auto colliderContainer = *_entity.get<ColliderContainer>();
	colliderContainer.DropAllContacts();
	_entity.set<ColliderContainer>(colliderContainer);

	flecsWorld->defer_end();
}
#pragma endregion

#pragma region Touch Events
void MAD::TileLogic::OnEnterTouch(TouchEventData _data)
{
	flecsWorld->defer_begin();

	flecs::entity _entity = flecsWorld->entity(ecs_get_name(*flecsWorld, _data.entityId));
	if (_entity.has<CrumblingPlatform>() && !_entity.has<Touched>())
	{
		_entity.add<Touched>();
		_entity.set_override<TimeTouched>({ GetNow() });
	}

	flecsWorld->defer_end();
}

void MAD::TileLogic::OnExitTouch(TouchEventData _data)
{
	flecsWorld->defer_begin();

	flecs::entity _entity = flecsWorld->entity(ecs_get_name(*flecsWorld, _data.entityId));
	if (_entity.has<CrumblingPlatform>() && !_entity.has<Crumbled>())
	{
		_entity.remove<Touched>();
		_entity.remove<RenderModel>();
		_entity.remove<Collidable>();
		auto colliderContainer = *_entity.get<ColliderContainer>();
		colliderContainer.DropAllContacts();
		_entity.set<ColliderContainer>(colliderContainer);
		_entity.add<Crumbled>();
		_entity.set_override<TimeCrumbled>({ GetNow() });
	}

	flecsWorld->defer_end();
}
#pragma endregion


#pragma region Activate / Shutdown
bool MAD::TileLogic::Activate(bool _runSystem)
{
	if (_runSystem)
	{
		springSystem.enable();
		strawberrySystem.enable();
		crystalCollectSystem.enable();
		crystalRespawnSystem.enable();
		graveSystem.enable();
		sceneExitSystem.enable();
		crumblingPlatformCrumbleSystem.enable();
		crumblingPlatformRespawnSystem.enable();
	}
	else
	{
		springSystem.disable();
		strawberrySystem.disable();
		crystalCollectSystem.disable();
		crystalRespawnSystem.disable();
		graveSystem.disable();
		sceneExitSystem.disable();
		crumblingPlatformCrumbleSystem.disable();
		crumblingPlatformRespawnSystem.disable();
	}

	return true;
}

bool MAD::TileLogic::Shutdown()
{
	springSystem.destruct();
	strawberrySystem.destruct();
	crystalCollectSystem.destruct();
	graveSystem.destruct();
	sceneExitSystem.destruct();
	crumblingPlatformCrumbleSystem.destruct();
	crumblingPlatformRespawnSystem.destruct();

	followingStrawberriesQuery.destruct();
	playerQuery.destruct();

	flecsWorld.reset();
	gameConfig.reset();
	saveLoader.reset();

	return true;
}
#pragma endregion
