#include "LevelEditorLogic.h"

#include "../Entities/Prefabs.h"

#include "../Components/SaveSlot.h"

#include "../Events/GameStateEvents.h"

#pragma region Init Main
bool MAD::LevelEditorLogic::Init(
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
	TileData* _tileData)
{
	window = _window;

	renderer = _renderer;
	tileData = _tileData;

	flecsWorld = _flecsWorld;
	// create an asynchronus version of the world
	flecsWorldAsync = flecsWorld->async_stage(); // just used for adding stuff, don't try to read data
	flecsWorldLock.Create();
	gameConfig = _gameConfig;

	keyboardMouseInput = _keyboardMouseInput;
	gameStateEventPusher = _gameStateEventPusher;
	editorEventPusher = _editorEventPusher;
	editorModeEventPusher = _editorModeEventPusher;
	editorErrorEventPusher = _editorErrorEventPusher;
	levelEventPusher = _levelEventPusher;

	saveLoader = _saveLoader;

	cameraQuery = flecsWorld->query<Camera, Transform>();
	cursorQuery = flecsWorld->query<LevelEditorCursor>();
	sceneCornerQuery = flecsWorld->query<SceneCorner>();
	newSceneCornerQuery = flecsWorld->query<NewSceneCorner>();
	renderInEditorQuery = flecsWorld->query<RenderInEditor>();

	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	defaultSceneHeight = readCfg->at("Scenes").at("defaultSceneHeight").as<UINT32>();
	defaultSceneWidth = readCfg->at("Scenes").at("defaultSceneWidth").as<UINT32>();

	sceneCornerScale = StringToGVector(readCfg->at("LevelEditor").at("sceneCornerScale").as<std::string>());
	newSceneCornerScale = StringToGVector(readCfg->at("LevelEditor").at("newSceneCornerScale").as<std::string>());
	camPanSensitivity = readCfg->at("LevelEditor").at("camPanSensitivity").as<float>();
	camZoomSensitivity = readCfg->at("LevelEditor").at("camZoomSensitivity").as<float>();

	curEditorMode = EDIT_TILEMAP;
	curTilesetId = 1;
	inLevelEditor = false;

	playerSceneIndex = saveLoader->GetSaveSlot().sceneIndex;

	SpawnCursor();
	SpawnAllSceneCorners();

	InitEditorSystem();
	InitEventHandlers();
	InitMergeAsyncSystem();

	return true;
}
#pragma endregion

#pragma region Init Event Handlers
void MAD::LevelEditorLogic::InitEventHandlers()
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
					UpdateCursorModel(curTilesetId);
					ShowEditorRenderables();

					std::cout << "Level editor on :)\n";
					break;
				}
				default:
				{
					if (inLevelEditor)
					{
						inLevelEditor = false;
						ResetEditorMode();
						HideEditorRenderables();

						std::cout << "Level editor off :(\n";

						if (unsavedScenes.size() > 0)
						{
							SaveScenes();
							std::cout << "Saving scene changes though.\n";
						}
					}
					break;
				}
				}
			}
		});
	gameStateEventPusher.Register(gameStateEventHandler);

	editorModeEventHandler.Create([this](const GW::GEvent& _event)
		{
			EDITOR_MODE event;
			EDITOR_MODE_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case EDIT_TILEMAP:
				std::cout << "Edit tilemap mode\n";
				break;
			case NEW_SCENE:
				std::cout << "New scene mode\n";
				break;
			default:
			{
				break;
			}
			}
		});
	editorModeEventPusher.Register(editorModeEventHandler);

	editorErrorEventHandler.Create([this](const GW::GEvent& _event)
		{
			EDITOR_ERROR_EVENT event;
			EDITOR_ERROR_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case NEW_SCENE_BAD_DIMENSIONS:
				std::cout << "Scene min corner is above and or to the right of the max corner.\n";
				break;
			case NEW_SCENE_COLLISION:
				std::cout << "One of the scene corners collides with scene " + std::to_string(data.value) + "\n";
				break;

			default:
			{
				break;
			}
			}
		});
	editorErrorEventPusher.Register(editorErrorEventHandler);

	levelEventHandler.Create([this](const GW::GEvent& _event)
		{
			LEVEL_EVENT event;
			LEVEL_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case LOAD_SCENE_DONE:
			{
				AddShownScene(data.sceneIndex, saveLoader->GetScene(data.sceneIndex));
				if (inLevelEditor)
					ShowEditorRenderables();
				break;
			}
			case UNLOAD_SCENE_DONE:
			{
				RemoveShownScene(data.sceneIndex);
				break;
			}
			case SHOW_SCENE:
			{
				AddShownScene(data.sceneIndex, saveLoader->GetScene(data.sceneIndex));
				if (inLevelEditor)
					ShowEditorRenderables();
				break;
			}
			case HIDE_SCENE:
			{
				RemoveShownScene(data.sceneIndex);
				break;
			}
			default:
			{
				break;
			}
			}
		});
	levelEventPusher.Register(levelEventHandler);
}
#pragma	endregion

#pragma region Init Systems
void MAD::LevelEditorLogic::InitEditorSystem()
{
	struct EditorSystem {};
	flecsWorld->entity("Editor System").add<EditorSystem>();
	editorSystem = flecsWorld->system<EditorSystem>().each([this](flecs::entity _entity, EditorSystem&)
		{
			if (!inLevelEditor)
				return;

			float add = 0, remove = 0;
			float pan = 0, zoomIn = 0, zoomOut = 0;
			float mouseX = 0, mouseY = 0, mouseDeltaX = 0, mouseDeltaY = 0;
			float newScene = 0, loadScene = 0;
			float numberInput = 0, numberValue = 0;
			float shiftInput = 0;

			bool isWindowFocused;
			window.IsFocus(isWindowFocused);

			if (isWindowFocused)
			{
				keyboardMouseInput.GetState(G_BUTTON_LEFT, add);
				keyboardMouseInput.GetState(G_BUTTON_RIGHT, remove);
				keyboardMouseInput.GetState(G_BUTTON_MIDDLE, pan);
				keyboardMouseInput.GetState(G_MOUSE_SCROLL_UP, zoomIn);
				keyboardMouseInput.GetState(G_MOUSE_SCROLL_DOWN, zoomOut);
				keyboardMouseInput.GetState(G_KEY_N, newScene);
				keyboardMouseInput.GetState(G_KEY_M, loadScene);
				keyboardMouseInput.GetState(G_KEY_1, numberInput); if (numberInput != 0) numberValue = 1;
				keyboardMouseInput.GetState(G_KEY_2, numberInput); if (numberInput != 0) numberValue = 2;
				keyboardMouseInput.GetState(G_KEY_3, numberInput); if (numberInput != 0) numberValue = 3;
				keyboardMouseInput.GetState(G_KEY_4, numberInput); if (numberInput != 0) numberValue = 4;
				keyboardMouseInput.GetState(G_KEY_5, numberInput); if (numberInput != 0) numberValue = 5;
				keyboardMouseInput.GetState(G_KEY_6, numberInput); if (numberInput != 0) numberValue = 6;
				keyboardMouseInput.GetState(G_KEY_7, numberInput); if (numberInput != 0) numberValue = 7;
				keyboardMouseInput.GetState(G_KEY_8, numberInput); if (numberInput != 0) numberValue = 8;
				keyboardMouseInput.GetState(G_KEY_9, numberInput); if (numberInput != 0) numberValue = 9;
				keyboardMouseInput.GetState(G_KEY_0, numberInput); if (numberInput != 0) numberValue = 10;
				keyboardMouseInput.GetState(G_KEY_LEFTSHIFT, shiftInput);

				keyboardMouseInput.GetMousePosition(mouseX, mouseY);

				GW::GReturn result = keyboardMouseInput.GetMouseDelta(mouseDeltaX, mouseDeltaY);

				if (result == GW::GReturn::REDUNDANT)
				{
					mouseDeltaX = 0;
					mouseDeltaY = 0;
				}
			}

			HandleEditorInput(
				_entity,
				add,
				remove,
				pan,
				zoomIn,
				zoomOut,
				mouseX,
				mouseY,
				mouseDeltaX,
				mouseDeltaY,
				newScene,
				loadScene,
				numberValue,
				shiftInput);

			isAddPressed = add != 0;
			isRemovePressed = remove != 0;
			isNewScenePressed = newScene != 0;
			isLoadScenePressed = loadScene != 0;
			isNumberPressed = numberValue != 0;

			if (isInputLockedUntilRelease && !isAddPressed && !isRemovePressed && !isNewScenePressed && !isLoadScenePressed)
				isInputLockedUntilRelease = false;
		});
}

void MAD::LevelEditorLogic::InitMergeAsyncSystem()
{
	struct MergeEditorStages {};
	flecsWorld->entity("MergeEditorStages").add<MergeEditorStages>();
	flecsWorld->system<MergeEditorStages>()
		.kind(flecs::OnLoad)
		.each([this](flecs::entity _entity, MergeEditorStages& _MergeEditorStages)
			{
				flecsWorldLock.LockSyncWrite();
				flecsWorldAsync.merge();
				flecsWorldLock.UnlockSyncWrite();
			});
}
#pragma endregion

#pragma region Editor Input
void MAD::LevelEditorLogic::HandleEditorInput(
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
	float _shift)
{
	// Get mouse position in world space
	GVECTORF worldMousePos;
	GVECTORF clampedMousePos;
	renderer->ScreenToWorldSpace(_mouseX, _mouseY, worldMousePos);
	clampedMousePos = {
		std::round(worldMousePos.x),
		std::round(worldMousePos.y),
		std::round(worldMousePos.z),
		1
	};

	// Cursor Control
	if (cursorQuery.count() > 0)
	{
		GMATRIXF cursorTransform = cursorQuery.first().get<Transform>()->value;
		cursorTransform.row4 = clampedMousePos;
		cursorQuery.first().set<Transform>({ cursorTransform });
	}

	// Camera Control
	if (_pan != 0)
	{
		GVECTORF mouseDelta{ -_mouseDeltaX, _mouseDeltaY };
		GVector::ScaleF(mouseDelta, camPanSensitivity * _entity.delta_time(), mouseDelta);
		GMATRIXF camTransform = cameraQuery.first().get<Transform>()->value;
		GVector::AddVectorF(camTransform.row4, mouseDelta, camTransform.row4);
		cameraQuery.first().set<Transform>({ camTransform });
	}
	else if (_zoomIn != 0)
	{
		GMATRIXF camTransform = cameraQuery.first().get<Transform>()->value;
		if (camTransform.row4.z < -2.5)
		{
			camTransform.row4.z += camZoomSensitivity * _entity.delta_time();
			cameraQuery.first().set<Transform>({ camTransform });
		}
	}
	else if (_zoomOut != 0)
	{
		GMATRIXF camTransform = cameraQuery.first().get<Transform>()->value;
		camTransform.row4.z -= camZoomSensitivity * _entity.delta_time();
		cameraQuery.first().set<Transform>({ camTransform });
	}

	// Edit Controls
	if (isInputLockedUntilRelease)
		return;

	if (_numberValue != 0) // Switch tile
	{
		if (curEditorMode == EDIT_TILEMAP)
			SwitchTilesets(unsigned(_numberValue + (10 * _shift)));
	}

	if (_add != 0) // Add input
	{
		switch (curEditorMode)
		{
		case EDIT_TILEMAP:
			AddTile(clampedMousePos);
			break;
		case NEW_SCENE:
			PlaceSceneCorner(clampedMousePos);
			break;
		case NEW_SPAWNPOINT:
			SelectSpawnpointScene(clampedMousePos);
			break;
		}
	}
	else if (_remove != 0) // Remove input
	{
		switch (curEditorMode)
		{
		case EDIT_TILEMAP:
			RemoveTile(clampedMousePos);
			break;
		case NEW_SCENE:
			DestroyNewSceneCorners();
			break;
		}
	}
	else if (_newScene != 0) // New scene input
	{
		switch (curEditorMode)
		{
		case EDIT_TILEMAP:
			EnterNewSceneMode();
			break;
		case NEW_SCENE:
			ExitNewSceneMode();
			break;
		}
	}
	else if (_loadScene != 0) // Load scene input
	{

	}
}

void MAD::LevelEditorLogic::WorldSpaceToSceneUnits(
	const GVECTORF& _worldPos,
	const std::shared_ptr<Tilemap>& _scene,
	int& _outSceneRow,
	int& _outSceneCol)
{
	_outSceneRow = std::round(_worldPos.y - _scene->originY);
	_outSceneCol = std::round(_worldPos.x - _scene->originX);
}

void MAD::LevelEditorLogic::WorldSpaceToSceneUnits(const GVECTORF& _worldPos, const GVECTORF& _sceneOrigin, int& _outSceneRow, int& _outSceneCol)
{
	_outSceneRow = std::round(_worldPos.y - _sceneOrigin.y);
	_outSceneCol = std::round(_worldPos.x - _sceneOrigin.x);
}

void MAD::LevelEditorLogic::ResetEditorMode()
{
	newSceneCorners.clear();
	curEditorMode = EDIT_TILEMAP;
}
#pragma endregion

#pragma region New Spawnpoint
void MAD::LevelEditorLogic::SelectSpawnpointScene(GVECTORF _worldPos)
{
	int otherSceneIndex = saveLoader->GetSceneAtPoint(_worldPos);
	if (otherSceneIndex == -1)
		return;

	int spawnpointSceneIndex = saveLoader->GetSceneAtPoint(newSpawnpointPos);
	if (spawnpointSceneIndex == -1)
	{
		isInputLockedUntilRelease = true;
		PushEditorModeEvent(EDIT_TILEMAP);
		return;
	}

	std::shared_ptr<Tilemap> spawnpointScene = saveLoader->GetScene(spawnpointSceneIndex);

	TilemapTile* spawnpointTile = spawnpointScene->GetTile(newSpawnpointPos);
	spawnpointTile->orientationId = otherSceneIndex;
	spawnpointScene->AddSpawnpoint(_worldPos, otherSceneIndex);
	LogChange(spawnpointSceneIndex);

	isInputLockedUntilRelease = true;
	PushEditorModeEvent(EDIT_TILEMAP);
}
#pragma endregion

#pragma region Tilemap Editing
void MAD::LevelEditorLogic::SwitchTilesets(unsigned _tilesetId)
{
	if (isNumberPressed || _tilesetId >= tileData->tilesetNames.size())
		return;

	curTilesetId = _tilesetId;

	if (tileData->IsIdOfTileset(curTilesetId, "SceneExit"))
		curTilesetType = SCENE_EXIT;
	else if (tileData->IsIdOfTileset(curTilesetId, "Spawnpoint"))
		curTilesetType = SPAWNPOINT;
	else
		curTilesetType = NORMAL_TILE;

	UpdateCursorModel(curTilesetId);
}

void MAD::LevelEditorLogic::AddTile(GVECTORF _worldPos)
{
	int curSceneIndex = GetLoadedSceneAtPoint(_worldPos);
	if (curSceneIndex == -1)
	{
		curSceneIndex = saveLoader->GetSceneAtPoint(_worldPos);

		if (curSceneIndex != -1)
		{
			LoadScene((USHORT)curSceneIndex);
			isInputLockedUntilRelease = true;
		}

		return;
	}
	std::shared_ptr<Tilemap> curScene = saveLoader->GetScene(curSceneIndex);

	int sceneRow, sceneCol;
	WorldSpaceToSceneUnits(_worldPos, curScene, sceneRow, sceneCol);

	const TilemapTile* tile = curScene->GetTile(sceneRow, sceneCol);
	if (tile == NULL || tile->tilesetId == curTilesetId)
		return;

	if (tile->tilesetId != 0)
		RemoveTile(_worldPos);

	USHORT orientation = 0;
	switch (curTilesetType)
	{
	case TilesetType::NORMAL_TILE:
		break;
	case TilesetType::SCENE_EXIT: // if this is a scene exit, store the scene to exit to in the orientation
	{
		int adjacentSceneIndex = GetAdjacentSceneAtPoint(_worldPos);
		if (adjacentSceneIndex == -1)
			return;
		orientation = (USHORT)adjacentSceneIndex;
		saveLoader->AddSceneNeighbor(curSceneIndex, adjacentSceneIndex);
		break;
	}
	case TilesetType::SPAWNPOINT:
	{
		newSpawnpointPos = _worldPos;
		isInputLockedUntilRelease = true;
		PushEditorModeEvent(NEW_SPAWNPOINT);
		break;
	}
	}

	curScene->tiles.at(sceneRow).at(sceneCol) = { curTilesetId, orientation };
	LogChange(curSceneIndex);

	EDITOR_EVENT_DATA eventData
	{
		(USHORT)curSceneIndex,
		sceneRow,
		sceneCol,
		curTilesetId,
		0,
	};

	PushEditorEvent(ADD_TILE, eventData);
}

void MAD::LevelEditorLogic::RemoveTile(GVECTORF _worldPos)
{
	int curSceneIndex = GetLoadedSceneAtPoint(_worldPos);
	if (curSceneIndex == -1)
	{
		curSceneIndex = saveLoader->GetSceneAtPoint(_worldPos);

		if (curSceneIndex != -1)
		{
			LoadScene((USHORT)curSceneIndex);
			isInputLockedUntilRelease = true;
		}

		return;
	}
	std::shared_ptr<Tilemap> curScene = saveLoader->GetScene(curSceneIndex);

	int sceneRow, sceneCol;
	WorldSpaceToSceneUnits(_worldPos, curScene, sceneRow, sceneCol);

	const TilemapTile* tile = curScene->GetTile(sceneRow, sceneCol);

	if (tile == NULL || tile->tilesetId == 0)
		return;

	TilemapTile previousTile = curScene->tiles.at(sceneRow).at(sceneCol);
	curScene->tiles.at(sceneRow).at(sceneCol) = { 0, 0 };

	switch (previousTile.tilesetId)
	{
	case SCENE_EXIT_ID:
		saveLoader->RemoveSceneNeighbor(curSceneIndex, previousTile.orientationId);
		break;
	case SPAWNPOINT_ID:
		curScene->RemoveSpawnpoint(previousTile.orientationId);
	}

	LogChange(curSceneIndex);

	EDITOR_EVENT_DATA eventData
	{
		(USHORT)curSceneIndex,
		sceneRow,
		sceneCol,
		previousTile.tilesetId,
		previousTile.orientationId
	};

	PushEditorEvent(REMOVE_TILE, eventData);
}
#pragma endregion

#pragma region Scenes Editing
int MAD::LevelEditorLogic::GetLoadedSceneAtPoint(GVECTORF _worldPosition)
{
	for (auto i = shownScenes.begin(); i != shownScenes.end(); i++)
	{
		if (i->second->IsPointInside(_worldPosition))
			return i->first;
	}

	return -1;
}

int MAD::LevelEditorLogic::GetAdjacentSceneAtPoint(GVECTORF _worldPos)
{
	int curSceneIndex = saveLoader->GetSceneAtPoint(_worldPos);
	if (curSceneIndex == -1)
		return -1;

	std::vector<USHORT> adjacentScenes;
	saveLoader->GetScenesAroundPoint(_worldPos, adjacentScenes);
	if (adjacentScenes.size() == 0)
		return -1;
	else
	{
		bool foundValidScene = false;
		for (USHORT index : adjacentScenes)
		{
			if (index != curSceneIndex)
			{
				return index;
			}
		}
	}

	return -1;
}


void MAD::LevelEditorLogic::SaveScenes()
{
	for (unsigned i : unsavedScenes)
	{
		saveLoader->SaveScene(i);
	}

	unsavedScenes.clear();
}

void MAD::LevelEditorLogic::LogChange(USHORT _sceneIndex)
{
	if (std::find(unsavedScenes.begin(), unsavedScenes.end(), _sceneIndex) == unsavedScenes.end())
		unsavedScenes.push_back(_sceneIndex);
}

void MAD::LevelEditorLogic::LoadScene(USHORT _sceneIndex)
{
	PushLevelEvent(LOAD_SCENE, { _sceneIndex });
	AddShownScene(_sceneIndex, saveLoader->GetScene(_sceneIndex));
}

void MAD::LevelEditorLogic::AddShownScene(USHORT _sceneIndex, std::shared_ptr<Tilemap> _scene)
{
	if (shownScenes.find(_sceneIndex) != shownScenes.end())
	{
		shownScenes.at(_sceneIndex) = _scene;
	}
	else
	{
		shownScenes.insert({ _sceneIndex, _scene });
	}
}

void MAD::LevelEditorLogic::RemoveShownScene(USHORT _sceneIndex)
{
	if (shownScenes.find(_sceneIndex) != shownScenes.end())
	{
		shownScenes.erase(_sceneIndex);
	}
}
#pragma endregion

#pragma region New Scene
void MAD::LevelEditorLogic::EnterNewSceneMode()
{
	if (isNewScenePressed)
		return;

	PushEditorModeEvent(NEW_SCENE);
	newSceneCorners.clear();
	UpdateCursorModel(1);
}

void MAD::LevelEditorLogic::ExitNewSceneMode()
{
	if (isNewScenePressed)
		return;

	PushEditorModeEvent(EDIT_TILEMAP);
	newSceneCorners.clear();
	DestroyNewSceneCorners();
	UpdateCursorModel(curTilesetId);
}

void MAD::LevelEditorLogic::NewScene()
{
	// Find min and max corners of the scene
	GVECTORF minCorner = newSceneCorners[0];
	GVECTORF maxCorner = newSceneCorners[0];

	if (newSceneCorners[1].x < minCorner.x)
		minCorner.x = newSceneCorners[1].x;
	else if (newSceneCorners[1].x > maxCorner.x)
		maxCorner.x = newSceneCorners[1].x;

	if (newSceneCorners[1].y < minCorner.y)
		minCorner.y = newSceneCorners[1].y;
	else if (newSceneCorners[1].y > maxCorner.y)
		maxCorner.y = newSceneCorners[1].y;

	// Create new scene
	int rows, cols;
	WorldSpaceToSceneUnits(maxCorner, minCorner, rows, cols);
	rows++;
	cols++;

	std::shared_ptr<Tilemap> tilemap = std::make_shared<Tilemap>((UINT32)minCorner.x, (UINT32)minCorner.y, rows, cols);
	USHORT newSceneIndex = saveLoader->AddNewScene(tilemap);
	SpawnSceneCorners(newSceneIndex);
	LogChange(newSceneIndex);
	LoadScene(newSceneIndex);
}

void MAD::LevelEditorLogic::PlaceSceneCorner(GVECTORF _worldPos)
{
	if (isAddPressed)
		return;
	if (saveLoader->GetSceneAtPoint(_worldPos) != -1)
		return;
	if (newSceneCorners.size() == 1 && (newSceneCorners[0].x == _worldPos.x || newSceneCorners[0].y == _worldPos.y))
		return;

	newSceneCorners.push_back(_worldPos);
	SpawnNewSceneCorner(_worldPos);

	if (newSceneCorners.size() == 2)
	{
		NewScene();
		ExitNewSceneMode();
	}
}

void MAD::LevelEditorLogic::SpawnNewSceneCorner(GVECTORF _worldPos)
{
	GMATRIXF transform = GIdentityMatrixF;
	GMatrix::ScaleLocalF(transform, newSceneCornerScale, transform);
	transform.row4 = _worldPos;

	flecsWorld->entity("SceneCorner")
		.add<NewSceneCorner>()
		.add<RenderInEditor>()
		.add<RenderModel>()
		.add<StaticModel>()
		.set_override<Transform>({ transform })
		.set_override<ModelOffset>({})
		.set_override<ModelIndex>({ tileData->GetTileModelIndex(1, 0) });
}

void MAD::LevelEditorLogic::DestroyNewSceneCorners()
{
	if (isRemovePressed)
		return;

	newSceneCorners.clear();

	std::vector<flecs::entity> sceneCorners;

	newSceneCornerQuery.each([&](flecs::entity _entity, NewSceneCorner&)
		{
			sceneCorners.push_back(_entity);
		});

	for (int i = 0; i < sceneCorners.size(); i++)
	{
		sceneCorners[i].destruct();
	}
}
#pragma endregion

#pragma region UI
void MAD::LevelEditorLogic::SpawnCursor()
{
	flecsWorldLock.LockSyncWrite();
	flecsWorldAsync.entity("LevelEditorCursor")
		.add<LevelEditorCursor>()
		.add<RenderInEditor>()
		.add<StaticModel>()
		.set_override<Transform>({ GIdentityMatrixF })
		.set_override <ModelOffset>({})
		.set_override<ModelIndex>({ 0 });
	flecsWorldLock.UnlockSyncWrite();
}

void MAD::LevelEditorLogic::UpdateCursorModel(unsigned _tilesetId)
{
	unsigned modelIndex = tileData->GetTileModelIndex(_tilesetId, 0);

	GMATRIXF transform = GIdentityMatrixF;

	flecs::entity tilePrefab;
	if (RetreivePrefab(tileData->GetTilePrefabName(_tilesetId, 0).c_str(), tilePrefab))
	{
		transform = tilePrefab.get<Transform>()->value;
		GMatrix::ScaleLocalF(transform, GVECTORF({ .8f, .8f, .8f }), transform);
	}

	flecsWorld->defer_begin();
	cursorQuery.each([&](flecs::entity _entity, LevelEditorCursor&)
		{
			_entity.set<ModelIndex>({ modelIndex });

			transform.row4 = _entity.get<Transform>()->value.row4;
			_entity.set<Transform>({ transform });
		});
	flecsWorld->defer_end();
}

void MAD::LevelEditorLogic::SpawnAllSceneCorners()
{
	auto scenes = saveLoader->GetAllScenes();

	flecsWorld->defer_begin();
	for (USHORT i = 0; i < scenes.size(); i ++)
	{
		SpawnSceneCorners(i);
	}
	flecsWorld->defer_end();
}

void MAD::LevelEditorLogic::SpawnSceneCorners(USHORT _sceneIndex)
{
	std::shared_ptr<Tilemap> scene = saveLoader->GetScene(_sceneIndex);

	GVECTORF bottomLeft = scene->GetMinCorner();
	GVECTORF topRight = scene->GetMaxCorner();
	GVECTORF topLeft{ bottomLeft.x, topRight.y };
	GVECTORF bottomRight{ topRight.x, bottomLeft.y };
	SpawnSceneCorner(_sceneIndex, 0, bottomLeft);
	SpawnSceneCorner(_sceneIndex, 1, topRight);
	SpawnSceneCorner(_sceneIndex, 2, topLeft);
	SpawnSceneCorner(_sceneIndex, 3, bottomRight);
}

void MAD::LevelEditorLogic::SpawnSceneCorner(USHORT _sceneIndex, USHORT _cornerIndex, GVECTORF _worldPos)
{
	std::string cornerName = "Corner." + std::to_string(_sceneIndex) + "." + std::to_string(_cornerIndex);
	GMATRIXF transform = GIdentityMatrixF;
	GMatrix::ScaleLocalF(transform, sceneCornerScale, transform);
	transform.row4.x = _worldPos.x;
	transform.row4.y = _worldPos.y;
	transform.row4.z = -1;

	flecsWorldLock.LockSyncWrite();
	flecs::entity spawnedTile = flecsWorldAsync.entity(cornerName.c_str())
		.add<SceneCorner>()
		.add<RenderInEditor>()
		.add<StaticModel>()
		.set_override<Transform>({ transform })
		.set_override<ModelOffset>({})
		.set_override<ModelIndex>({ tileData->GetTileModelIndex(1, 0) });

	if (inLevelEditor)
		spawnedTile.add<RenderModel>();
	flecsWorldLock.UnlockSyncWrite();
}

void MAD::LevelEditorLogic::ShowEditorRenderables()
{
	flecsWorld->defer_begin();
	renderInEditorQuery.each([this](flecs::entity _entity, RenderInEditor&)
		{
			if (!_entity.has<RenderModel>())
			{
				if (!_entity.has<Tile>() || IsSceneShown(_entity.get<Tile>()->sceneIndex))
					_entity.add<RenderModel>();
			}
		});
	flecsWorld->defer_end();
}

void MAD::LevelEditorLogic::HideEditorRenderables()
{
	flecsWorld->defer_begin();
	renderInEditorQuery.each([](flecs::entity _entity, RenderInEditor&)
		{
			if (_entity.has<RenderModel>())
			{
				_entity.remove<RenderModel>();
			}
		});
	flecsWorld->defer_end();
}
#pragma endregion

#pragma region Event Pushers
void MAD::LevelEditorLogic::PushEditorEvent(EDITOR_EVENT _event, EDITOR_EVENT_DATA _eventData)
{
	GW::GEvent editorEvent;
	editorEvent.Write(_event, _eventData);
	editorEventPusher.Push(editorEvent);
}

void MAD::LevelEditorLogic::PushEditorModeEvent(EDITOR_MODE _event, EDITOR_MODE_EVENT_DATA _eventData)
{
	curEditorMode = _event;
	isInputLockedUntilRelease = true;

	GW::GEvent modeEvent;
	modeEvent.Write(_event, _eventData);
	editorModeEventPusher.Push(modeEvent);
}

void MAD::LevelEditorLogic::PushEditorErrorEvent(EDITOR_ERROR_EVENT _event, EDITOR_ERROR_EVENT_DATA _eventData)
{
	GW::GEvent errorEvent;
	errorEvent.Write(_event, _eventData);
	editorModeEventPusher.Push(errorEvent);
}

void MAD::LevelEditorLogic::PushLevelEvent(LEVEL_EVENT _event, LEVEL_EVENT_DATA _eventData)
{
	GW::GEvent errorEvent;
	errorEvent.Write(_event, _eventData);
	levelEventPusher.Push(errorEvent);
}

bool MAD::LevelEditorLogic::IsSceneShown(USHORT _sceneIndex)
{
	return shownScenes.find(_sceneIndex) != shownScenes.end();
}
#pragma endregion

#pragma region Activate / Shutdown / Run
void MAD::LevelEditorLogic::Reset()
{
	shownScenes.clear();
}

bool MAD::LevelEditorLogic::Activate(bool _runSystem)
{
	if (_runSystem)
	{
		flecsWorld->entity("MergeEditorStages").enable();
	}
	else
	{
		flecsWorld->entity("MergeEditorStages").disable();
	}

	return true;
}

bool MAD::LevelEditorLogic::Shutdown()
{
	flecsWorldAsync.merge();

	cameraQuery.destruct();
	cursorQuery.destruct();
	sceneCornerQuery.destruct();
	newSceneCornerQuery.destruct();
	renderInEditorQuery.destruct();

	flecsWorld.reset();
	gameConfig.reset();
	saveLoader.reset();

	return true;
}
#pragma endregion
