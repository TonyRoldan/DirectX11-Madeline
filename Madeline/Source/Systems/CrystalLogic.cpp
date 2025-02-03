//#include "CrystalLogic.h"
//
//#include "../Components/Identification.h"
//#include "../Components/Physics.h"
//#include "../Components/Tilemaps.h"
//
//#pragma region Init
//bool MAD::CrystalLogic::Init(
//	std::shared_ptr<flecs::world> _flecsWorld,
//	GW::CORE::GEventGenerator _levelEventPusher,
//	std::shared_ptr<SaveLoader> _saveLoader)
//{
//	flecsWorld = _flecsWorld;
//
//	levelEventPusher = _levelEventPusher;
//
//	saveLoader = _saveLoader;
//
//	InitEventHandlers();
//	InitSceneExitSystem();
//
//	return true;
//}
//#pragma endregion
//
//#pragma region Event Handlers
//void MAD::TriggerLogic::InitEventHandlers()
//{
//}
//#pragma endregion
//
//#pragma region Scene Exit System
//void MAD::CrystalLogic::InitSceneExitSystem()
//{
//	sceneExitSystem = flecsWorld->system<SceneExit, Tile, ColliderContainer, Collidable>()
//		.each([this](flecs::entity _entity, SceneExit&, Tile& _tile, ColliderContainer& _colliderContainer, Collidable&)
//			{
//				if (_colliderContainer.triggerColliders[0]->contacts.size() > 0)
//				{
//					PushLevelEvent(HIT_SCENE_EXIT, { saveLoader->GetScene(_tile.sceneIndex)->GetTile(_tile)->orientationId, _entity });
//				}
//			});
//}
//#pragma endregion
//
//#pragma region Event Pushers
//void MAD::CrystalLogic::PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _data)
//{
//	GW::GEvent levelEvent;
//	levelEvent.Write(_event, _data);
//	levelEventPusher.Push(levelEvent);
//}
//#pragma endregion
//
//#pragma region Activate / Shutdown
//bool MAD::CrystalLogic::Activate(bool _runSystem)
//{
//	return false;
//}
//
//bool MAD::CrystalLogic::Shutdown()
//{
//	return false;
//}
//#pragma endregion