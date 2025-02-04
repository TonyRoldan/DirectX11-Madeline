#ifndef CAMERA_LOGIC_H
#define CAMERA_LOGIC_H
#include "../GameConfig.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Systems/Renderer.h"
#include "../Components/Gameplay.h"

#include "../Loaders/SaveLoader.h"

#include "../Events/CameraEvents.h"

namespace MAD
{
	class CameraLogic
	{
		std::shared_ptr<flecs::world> flecsWorld;

		std::weak_ptr<const GameConfig> gameConfig;

		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventGenerator cameraEventPusher;
		GW::CORE::GEventResponder gameStateEventHandler;
		GW::CORE::GEventResponder cameraEventHandler;

		std::shared_ptr<SaveLoader> saveLoader;

		DirectX11Renderer* renderer;

		flecs::system movementSystem;

		flecs::query<Player, Transform> playerTransformQuery;

		float smoothing;
		float zOffset;

		GW::MATH::GMATRIXF defaultPosition;
		GW::MATH::GVECTORF basePosition;
		GW::MATH::GVECTORF safeArea;

		bool isCameraShaking;
		GW::MATH::GVECTORF maxShakeOffset;
		GW::MATH::GVECTORF minShakeOffset;
		GW::MATH::GVECTORF shakeOffset;
		float maxShakeTime;
		float shakeTime;

		bool inLevelEditor;


	public:
		bool Init(
			std::shared_ptr<flecs::world> _flecsWorld,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			GW::CORE::GEventGenerator _cameraEventPusher,
			std::shared_ptr<SaveLoader> _saveLoader,
			DirectX11Renderer* _renderer);

	private:
		void InitEventResponders();
		void InitCameraSystem();

		void OnCameraShake(CameraEventData _data);

		GW::MATH::GVECTORF ClampToSafeArea(GW::MATH::GVECTORF _pos);

	public:
		void Reset();
		bool Activate(bool runSystem);
		bool Shutdown();
	};
}
#endif