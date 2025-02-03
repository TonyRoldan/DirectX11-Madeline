#ifndef LEVELEDITORLOGIC_H
#define LEVELEDITORLOGIC_H

#include "../GameConfig.h"
#include "../Entities/TileData.h"
#include "../Loaders/SaveLoader.h"
#include "../Events/EditorEvents.h"
#include "../Events/LevelEvents.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Systems/Renderer.h"

namespace MAD
{
	class LevelEditorLogic
	{
		enum TilesetType { NORMAL_TILE, SCENE_EXIT, SPAWNPOINT };

		GW::SYSTEM::GWindow window;

		DirectX11Renderer* renderer;

		std::shared_ptr<flecs::world> flecsWorld;
		flecs::world flecsWorldAsync;
		GW::CORE::GThreadShared flecsWorldLock;

		std::weak_ptr<const GameConfig> gameConfig;

		GW::INPUT::GInput keyboardMouseInput;

		GW::CORE::GEventGenerator gameStateEventPusher;
		GW::CORE::GEventGenerator editorEventPusher;
		GW::CORE::GEventGenerator editorModeEventPusher;
		GW::CORE::GEventGenerator editorErrorEventPusher;
		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventResponder gameStateEventHandler;
		GW::CORE::GEventResponder editorModeEventHandler;
		GW::CORE::GEventResponder editorErrorEventHandler;
		GW::CORE::GEventResponder levelEventHandler;


		std::shared_ptr<SaveLoader> saveLoader;

		TileData* tileData;

		flecs::system editorSystem;

		flecs::query<Camera, Transform> cameraQuery;
		flecs::query<LevelEditorCursor> cursorQuery;
		flecs::query<SceneCorner> sceneCornerQuery;
		flecs::query<NewSceneCorner> newSceneCornerQuery;
		flecs::query<RenderInEditor> renderInEditorQuery;

		std::unordered_map<USHORT, std::shared_ptr<Tilemap>> shownScenes;
		std::vector<USHORT> unsavedScenes;

		int playerSceneIndex;
		USHORT curTilesetId;
		TilesetType curTilesetType;

		UINT32 defaultSceneHeight;
		UINT32 defaultSceneWidth;

		std::vector<GVECTORF> newSceneCorners;
		GVECTORF newSpawnpointPos;

		GVECTORF sceneCornerScale;
		GVECTORF newSceneCornerScale;

		float camPanSensitivity;
		float camZoomSensitivity;

		bool isAddPressed;
		bool isRemovePressed;
		bool isNewScenePressed;
		bool isLoadScenePressed;
		bool isNumberPressed;
		bool isShiftPressed;

		EDITOR_MODE curEditorMode;
		bool isInputLockedUntilRelease;
		bool inLevelEditor;

	public:
		bool Init(
			GW::SYSTEM::GWindow _window,
			DirectX11Renderer* _renderer,
			std::shared_ptr<flecs::world> _flecsWorld,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::INPUT::GInput _keyboardMouseInput,
			GW::CORE::GEventGenerator _gameStateEventPusher,
			GW::CORE::GEventGenerator _editorEventPusher,
			GW::CORE::GEventGenerator _editorModeEventPusher,
			GW::CORE::GEventGenerator _editorErrorEventPusher,
			GW::CORE::GEventGenerator _levelEventPusher,
			std::shared_ptr<SaveLoader> _saveLoader,
			TileData* _tileData);

	private:
		void InitEventHandlers();
		void InitEditorSystem();
		void InitMergeAsyncSystem();

		void HandleEditorInput(
			flecs::entity _entity,
			float _add, 
			float _remove, 
			float _pan, 
			float _zoomIn, 
			float _zoomOut, 
			float _mouseX, 
			float _mouseY,
			float _mouseDeltaX,
			float _mouseDeltaY,
			float _newScene,
			float _loadScene,
			float _numberValue,
			float _shift);

		void WorldSpaceToSceneUnits(
			const GVECTORF& _worldPos, 
			const std::shared_ptr<Tilemap>& _scene, 
			int& _outSceneRow, 
			int& _outSceneCol);
		void WorldSpaceToSceneUnits(
			const GVECTORF& _worldPos,
			const GVECTORF& _sceneOrigin,
			int& _outSceneRow,
			int& _outSceneCol);

		void SwitchTilesets(unsigned _tilesetId);
		void AddTile(GVECTORF _worldPos);
		void RemoveTile(GVECTORF _worldPos);
		int GetLoadedSceneAtPoint(GVECTORF _worldPos);
		int GetAdjacentSceneAtPoint(GVECTORF _worldPos);

		void SaveScenes();
		void LogChange(USHORT _sceneIndex);

		void LoadScene(USHORT _sceneIndex);
		void AddShownScene(USHORT _sceneIndex, std::shared_ptr<Tilemap> _scene);
		void RemoveShownScene(USHORT _sceneIndex);

		// New Scene
		void EnterNewSceneMode();
		void ExitNewSceneMode();
		void NewScene();
		void PlaceSceneCorner(GVECTORF _worldPos);
		void SpawnNewSceneCorner(GVECTORF _worldPos);
		void DestroyNewSceneCorners();

		void ResetEditorMode();

		// New Spawnpoint
		void SelectSpawnpointScene(GVECTORF _worldPos);

		// UI
		void SpawnCursor();
		void UpdateCursorModel(unsigned _tilesetId);
		void SpawnAllSceneCorners();
		void SpawnSceneCorners(USHORT _sceneIndex);
		void SpawnSceneCorner(USHORT _sceneIndex, USHORT _cornerIndex, GVECTORF _worldPos);
		void ShowEditorRenderables();
		void HideEditorRenderables();

		// Event Pushers
		void PushEditorEvent(EDITOR_EVENT _event, EDITOR_EVENT_DATA _eventData);
		void PushEditorModeEvent(EDITOR_MODE _event, EDITOR_MODE_EVENT_DATA _eventData = {});
		void PushEditorErrorEvent(EDITOR_ERROR_EVENT _event, EDITOR_ERROR_EVENT_DATA _eventData = {});
		void PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _eventData);

		bool IsSceneShown(USHORT _sceneIndex);

	public:
		void Reset();

		bool Activate(bool _runSystem);

		bool Shutdown();
	};
}
#endif