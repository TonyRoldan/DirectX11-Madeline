#include "CameraLogic.h"
#include "../Utils/Macros.h"
#include "../Events/CameraEvents.h"

bool MAD::CameraLogic::Init(
	std::shared_ptr<flecs::world> _flecsWorld,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::CORE::GEventGenerator _gameStateEventPusher,
	GW::CORE::GEventGenerator _cameraEventPusher,
	std::shared_ptr<SaveLoader> _saveLoader,
	DirectX11Renderer* _renderer)
{
	flecsWorld = _flecsWorld;
	gameStateEventPusher = _gameStateEventPusher;
	cameraEventPusher = _cameraEventPusher;
	renderer = _renderer;
	saveLoader = _saveLoader;

	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	// Camera entity creation
	smoothing = readCfg->at("Camera").at("smoothing").as<float>();
	GW::MATH::GVECTORF pos = StringToGVector(readCfg->at("Camera").at("position").as<std::string>());
	GW::MATH::GVECTORF dir = StringToGVector(readCfg->at("Camera").at("rotation").as<std::string>());
	safeArea = StringToGVector(readCfg->at("Camera").at("safeArea").as<std::string>());

	defaultPosition = { GW::MATH::GIdentityMatrixF };
	GW::MATH::GMatrix::RotateXLocalF(defaultPosition, G_DEGREE_TO_RADIAN_F(dir.x), defaultPosition);
	GW::MATH::GMatrix::RotateYLocalF(defaultPosition, G_DEGREE_TO_RADIAN_F(dir.y), defaultPosition);
	GW::MATH::GMatrix::RotateZLocalF(defaultPosition, G_DEGREE_TO_RADIAN_F(dir.z), defaultPosition);
	defaultPosition.row4 = pos;
	basePosition = defaultPosition.row4;
	shakeOffset = {};

	zOffset = pos.z;

	isCameraShaking = false;
	inLevelEditor = false;

	flecsWorld->entity("Camera")
		.add<Camera>()
		.set_override<Transform>({ defaultPosition });

	// Camera movement system creation
	playerTransformQuery = flecsWorld->query<Player, Transform>();

	InitCameraSystem();
	InitEventResponders();

	return true;
}

void MAD::CameraLogic::InitEventResponders()
{
	gameStateEventHandler.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE event;
			GAME_STATE_EVENT_DATA data;

			if (+_event.Read(event, data))
			{
				switch (event)
				{
				case GAME_STATE::LEVEL_EDITOR:
				{
					inLevelEditor = true;
					break;
				}
				default:
				{
					if (inLevelEditor)
					{
						inLevelEditor = false;
					}
					break;
				}
				}
			}
		});
	gameStateEventPusher.Register(gameStateEventHandler);

	cameraEventHandler.Create([this](const GW::GEvent& _event)
		{
			CameraEvent event;
			CameraEventData data;

			if (+_event.Read(event, data))
			{
				switch (event)
				{
				case MAD::SHAKE:
				{
					OnCameraShake(data);
					break;
				}
				}
			}
		});
	cameraEventPusher.Register(cameraEventHandler);
}

void MAD::CameraLogic::InitCameraSystem()
{
	movementSystem = flecsWorld->system<Camera, Transform>("CameraMovementSystem").each([this](
		flecs::entity _entity, Camera, Transform& _transform)
		{
			if (!inLevelEditor && playerTransformQuery.count() > 0)
			{
				// Follow player
				GW::MATH::GVECTORF targetPos = playerTransformQuery.first().get<Transform>()->value.row4;
				targetPos.z += zOffset;
				targetPos = ClampToSafeArea(targetPos);

				GW::MATH::GVector::LerpF(
					basePosition,
					targetPos,
					smoothing * _entity.delta_time(),
					basePosition);

				// Camera shake
				if (isCameraShaking)
				{
					shakeTime += _entity.delta_time();

					if (shakeTime < maxShakeTime)
					{
						float offsetRatio = shakeTime / maxShakeTime;
						float shakeRatio = (1 + sin((offsetRatio) * 3 * PI)) * .5;
						GW::MATH::GVECTORF maxAdjustedOffset;
						GW::MATH::GVECTORF minAdjustedOffset;
						GW::MATH::GVector::LerpF(maxShakeOffset, GW::MATH::GVECTORF({}), offsetRatio, maxAdjustedOffset);
						GW::MATH::GVector::LerpF(minShakeOffset, GW::MATH::GVECTORF({}), offsetRatio, minAdjustedOffset);
						GW::MATH::GVector::LerpF(minAdjustedOffset, maxAdjustedOffset, shakeRatio, shakeOffset);
					}
					else 
					{
						isCameraShaking = false;
						shakeOffset = {};
					}
				}

				GW::MATH::GVector::AddVectorF(basePosition, shakeOffset, _transform.value.row4);
			}

			renderer->UpdateCamera(_transform.value);
		});
}

void MAD::CameraLogic::OnCameraShake(CameraEventData _data)
{
	isCameraShaking = true;
	GW::MATH::GVector::ScaleF(_data.direction, _data.distance, maxShakeOffset);
	GW::MATH::GVector::ScaleF(_data.direction, -_data.distance, minShakeOffset);
	maxShakeTime = _data.time;
	shakeTime = 0;
}

GW::MATH::GVECTORF MAD::CameraLogic::ClampToSafeArea(GW::MATH::GVECTORF _pos)
{
	unsigned sceneIndex = saveLoader->GetSaveSlot().sceneIndex;
	std::shared_ptr<Tilemap> scene = saveLoader->GetScene(sceneIndex);

	if (scene == NULL)
		return GZeroVectorF;

	_pos.x -= scene->originX;
	_pos.y -= scene->originY;

	if (scene->columns < safeArea.x * 2)
		_pos.x = (float)scene->columns * .5f;
	else if (_pos.x < safeArea.x)
		_pos.x = safeArea.x;
	else if (_pos.x > scene->columns - safeArea.x)
		_pos.x = scene->columns - safeArea.x;

	if (scene->rows < safeArea.y * 2)
		_pos.y = (float)scene->rows * .5f;
	else if (_pos.y < safeArea.y)
		_pos.y = safeArea.y;
	else if (_pos.y > scene->rows - safeArea.y)
		_pos.y = scene->rows - safeArea.y;

	_pos.x += scene->originX;
	_pos.y += scene->originY;

	return _pos;
}

void MAD::CameraLogic::Reset()
{
	flecsWorld->entity("Camera")
		.set<Transform>({ defaultPosition });
	basePosition = defaultPosition.row4;
	shakeOffset = {};
}

bool MAD::CameraLogic::Activate(bool runSystem)
{
	if (movementSystem.is_alive()) {
		(runSystem) ?
			movementSystem.enable()
			: movementSystem.disable();
		return true;
	}
	return false;
}

bool MAD::CameraLogic::Shutdown()
{

	movementSystem.destruct();
	playerTransformQuery.destruct();
	flecsWorld.reset();
	gameConfig.reset();

	return true;
}