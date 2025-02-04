#include "Renderer.h"


using namespace MAD;

#pragma region Initialize

#pragma region Init
bool MAD::DirectX11Renderer::Init(GW::SYSTEM::GWindow _win,
	GW::GRAPHICS::GDirectX11Surface _renderingSurface,
	std::shared_ptr<flecs::world> _flecsWorld,
	std::shared_ptr<flecs::world> _uiWorld,
	std::weak_ptr<const GameConfig> _gameConfig,
	std::shared_ptr<ModelLoader> _models)
{
	window = _win;
	d3d = _renderingSurface;
	flecsWorld = _flecsWorld;
	uiWorld = _uiWorld;
	gameConfig = _gameConfig;
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	modelLoader = _models;

	input.Create(_win);

	//World
	GW::MATH::GMATRIXF world{ GW::MATH::GIdentityMatrixF };
	worldMatrix = world;

	//View
	GW::MATH::GVECTORF eye{ 13.0f, 21.0f, 13.0f, 1.0f };
	GW::MATH::GVECTORF at{ 0.15f, 0.75f, 0.0f, 1.0f };
	GW::MATH::GVECTORF up{ 0.0f, 1.0f, 0.0f, 1.0f };
	GW::MATH::GMATRIXF view;
	GW::MATH::GMatrix::LookAtLHF(eye, at, up, view);
	viewMatrix = view;
	camPos = eye;

	//Projection
	fov = 65.0f * 3.14f / 180.0f;
	d3d.GetAspectRatio(aspect);
	nearPlane = 0.1f;
	farPlane = 2000.0f;
	GW::MATH::GMATRIXF projection;
	GW::MATH::GMatrix::ProjectionDirectXLHF(fov, aspect, nearPlane, farPlane, projection);
	projectionMatrix = projection;

	//Models
	modelAttribute = Attributes();
	for (int i = 0; i < instanceMax; i++)
	{
		instanceTransforms.transforms[i] = world;
	}

	for (int i = 0; i < instanceMax; i++)
	{
		instanceColliders.boxColliders[i].min = { 0.0f,0.0f,0.0f,0.0f };
		instanceColliders.boxColliders[i].max = { 0.0f,0.0f,0.0f,0.0f };
	}

	for (int i = 0; i < 100; i++)
	{
		instancePose.pose.push_back(GW::MATH::GIdentityMatrixF);
	}

	screenWidth = readCfg->at("Window").at("width").as<int>();
	screenHeight = readCfg->at("Window").at("height").as<int>();

	gameViewport.TopLeftX = 0;
	gameViewport.TopLeftY = 0;
	gameViewport.Width = screenWidth;
	gameViewport.Height = screenHeight;
	gameViewport.MinDepth = 0;
	gameViewport.MaxDepth = 1;


	black = { 0.0f, 0.0f, 0.0f, 1.0f };
	white = { 1.0f, 1.0f, 1.0f, 1.0f };
	


	rust = { 0.72f, 0.25f, 0.05f, 1.0f };
	bgColor = white;

	lastUpdate = std::chrono::steady_clock::now();

	modelElapsedTime = 0.0f;
	modelAnimSpeed = 0.5f;
	transitionLength = 0.0f;
	animationState = { 4, 4, 0.0f, false };
	modelAnimPause = false;

	playerCurrScore = 0;

	uiScalar = 1;	

	isDebugOn = false;

	Quad myQuad = gameScreen;

	//Buffer Data

	instanceData = { 0, 0, texId };

	sceneData = { };
	sceneData.viewMatrix = viewMatrix;
	sceneData.projectionMatrix = projectionMatrix;
	sceneData.camPos = camPos;
	sceneData.dirLightColor = { 0.75f, 0.75f, 1.0f, 1.0f };
	sceneData.dirLightDir = { -2.0f, -2.0f, 0.0f, 1.0f };
	sceneData.ambientTerm = { 0.5f, 0.4f, 0.3f, 0.0f };
	sceneData.fogColor = { 0.5f, 0.5f, 1.1f, 1 };
	sceneData.fogDensity = 0.005;
	sceneData.fogStartDistance = 75;
	sceneData.contrast = 1;
	sceneData.saturation = 1.1;

	meshData = { worldMatrix, modelAttribute, 0 };

	modelQuery = flecsWorld->query<const RenderModel, const ModelIndex, const Moveable>();
	animationQuery = flecsWorld->query<const RenderModel, const AnimateModel, const Moveable, const ModelIndex>();
	levelQuery = flecsWorld->query<const RenderModel, const ModelIndex, const StaticModel>();
	spriteQuery = uiWorld->query<const RenderSprite, Sprite>();
	textQuery = uiWorld->query<const RenderText, Text>();


	if (LoadShaders() == false)
		return false;
	if (LoadTextures() == false)
		return false;
	LoadBuffers();
	LoadShaderResources();
	LoadGeometry();
	if (SetupPipeline() == false)
		return false;
	InitRendererSystems();

	return true;
}

#pragma endregion

#pragma region Shaders

bool MAD::DirectX11Renderer::LoadShaders()
{
	ID3D11Device* device{};
	d3d.GetDevice((void**)&device);

	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();

	std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");
	std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");
	std::string quadVSSource = ReadFileIntoString("../Shaders/VSMap.hlsl");
	std::string quadPSSource = ReadFileIntoString("../Shaders/PSMap.hlsl");
	std::string mapModelsPSSource = ReadFileIntoString("../Shaders/PSColorMapModels.hlsl");
	std::string levelVSSource = ReadFileIntoString("../Shaders/VSLevelShader.hlsl");
	std::string debugVSSource = ReadFileIntoString("../Shaders/VSDebug.hlsl");
	std::string debugPSSource = ReadFileIntoString("../Shaders/PSDebug.hlsl");
	std::string debugGSSource = ReadFileIntoString("../Shaders/GSDebug.hlsl");

	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT vsCompilationResult = D3DCompile(vertexShaderSource.c_str(),
		vertexShaderSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		compilerFlags,
		0,
		vsBlob.GetAddressOf(),
		errors.GetAddressOf());

	HRESULT vsQuadCompResult = D3DCompile(quadVSSource.c_str(),
		quadVSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		compilerFlags,
		0,
		vsQuadBlob.GetAddressOf(),
		errors.GetAddressOf());

	HRESULT vsLevelCompResult = D3DCompile(levelVSSource.c_str(),
		levelVSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		compilerFlags,
		0,
		vsLevelBlob.GetAddressOf(),
		errors.GetAddressOf());

	HRESULT vsDebugCompResult = D3DCompile(debugVSSource.c_str(),
		debugVSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		compilerFlags,
		0,
		vsDebugBlob.GetAddressOf(),
		errors.GetAddressOf());

	if (SUCCEEDED(vsCompilationResult))
	{
		device->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			nullptr, vertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	if (SUCCEEDED(vsQuadCompResult))
	{
		device->CreateVertexShader(vsQuadBlob->GetBufferPointer(),
			vsQuadBlob->GetBufferSize(),
			nullptr, quadVertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	if (SUCCEEDED(vsLevelCompResult))
	{
		device->CreateVertexShader(vsLevelBlob->GetBufferPointer(),
			vsLevelBlob->GetBufferSize(),
			nullptr, levelVertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	if (SUCCEEDED(vsDebugCompResult))
	{
		device->CreateVertexShader(vsDebugBlob->GetBufferPointer(),
			vsDebugBlob->GetBufferSize(),
			nullptr, debugVertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	HRESULT psCompilationResult = D3DCompile(pixelShaderSource.c_str(),
		pixelShaderSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		compilerFlags,
		0,
		psBlob.GetAddressOf(),
		errors.GetAddressOf());

	HRESULT psQuadCompResult = D3DCompile(quadPSSource.c_str(),
		quadPSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		compilerFlags,
		0,
		psQuadBlob.GetAddressOf(),
		errors.GetAddressOf());

	HRESULT psDebugCompResult = D3DCompile(debugPSSource.c_str(),
		debugPSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		compilerFlags,
		0,
		psDebugBlob.GetAddressOf(),
		errors.GetAddressOf());

	if (SUCCEEDED(psCompilationResult))
	{
		device->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(),
			nullptr,
			pixelShader.GetAddressOf());

	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	if (SUCCEEDED(psQuadCompResult))
	{
		device->CreatePixelShader(psQuadBlob->GetBufferPointer(),
			psQuadBlob->GetBufferSize(),
			nullptr,
			quadPixelShader.GetAddressOf());

	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	if (SUCCEEDED(psDebugCompResult))
	{
		device->CreatePixelShader(psDebugBlob->GetBufferPointer(),
			psDebugBlob->GetBufferSize(),
			nullptr,
			debugPixelShader.GetAddressOf());

	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	HRESULT gsDebugCompResult = D3DCompile(debugGSSource.c_str(),
		debugGSSource.length(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"gs_5_0",
		compilerFlags,
		0,
		gsBlob.GetAddressOf(),
		errors.GetAddressOf());

	if (SUCCEEDED(gsDebugCompResult))
	{
		device->CreateGeometryShader(gsBlob->GetBufferPointer(),
			gsBlob->GetBufferSize(),
			nullptr,
			geometryShader.GetAddressOf());

	}
	else
	{
		PrintLabeledDebugString("Geometry Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return false;
	}

	device->Release();

	return true;
}

#pragma endregion

#pragma region Buffers
bool MAD::DirectX11Renderer::LoadBuffers()
{
	ID3D11Device* device;
	d3d.GetDevice((void**)&device);

	D3D11_BUFFER_DESC cbMeshDesc{};
	cbMeshDesc.ByteWidth = sizeof(MeshData);
	cbMeshDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbMeshDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbMeshDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbMeshDesc.MiscFlags = 0;
	cbMeshDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA meshSubData{};
	meshSubData.pSysMem = &meshData;
	meshSubData.SysMemPitch = 0;
	meshSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC cbSceneDesc{};
	cbSceneDesc.ByteWidth = sizeof(SceneData);
	cbSceneDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbSceneDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbSceneDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbSceneDesc.MiscFlags = 0;
	cbSceneDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA sceneSubData{};
	sceneSubData.pSysMem = &sceneData;
	sceneSubData.SysMemPitch = 0;
	sceneSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC cbInstanceDesc{};
	cbInstanceDesc.ByteWidth = sizeof(PerInstanceData);
	cbInstanceDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbInstanceDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbInstanceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbInstanceDesc.MiscFlags = 0;
	cbInstanceDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA instanceSubData{};
	instanceSubData.pSysMem = &instanceData;
	instanceSubData.SysMemPitch = 0;
	instanceSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC sbTransformDesc{};
	sbTransformDesc.ByteWidth = sizeof(TransformData) * instanceMax;
	sbTransformDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbTransformDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbTransformDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbTransformDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbTransformDesc.StructureByteStride = sizeof(TransformData);

	D3D11_SUBRESOURCE_DATA transformSubData{};
	transformSubData.pSysMem = &instanceTransforms.transforms;
	transformSubData.SysMemPitch = 0;
	transformSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC sbColliderDesc{};
	sbColliderDesc.ByteWidth = sizeof(GW::MATH::GAABBMMF) * instanceMax;
	sbColliderDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbColliderDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbColliderDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbColliderDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbColliderDesc.StructureByteStride = sizeof(GW::MATH::GAABBMMF);

	D3D11_SUBRESOURCE_DATA colliderSubData{};
	colliderSubData.pSysMem = &instanceColliders.boxColliders;
	colliderSubData.SysMemPitch = 0;
	colliderSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC sbBonePoseDesc{};
	sbBonePoseDesc.ByteWidth = sizeof(TransformData) * 100;
	sbBonePoseDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbBonePoseDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbBonePoseDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbBonePoseDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbBonePoseDesc.StructureByteStride = sizeof(TransformData);

	D3D11_SUBRESOURCE_DATA bonePoseSubData{};
	bonePoseSubData.pSysMem = instancePose.pose.data();
	bonePoseSubData.SysMemPitch = 0;
	bonePoseSubData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC sbLightDesc{};
	sbLightDesc.ByteWidth = sizeof(PointLight) * lightInstanceMax;
	sbLightDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbLightDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbLightDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbLightDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbLightDesc.StructureByteStride = sizeof(PointLight);

	D3D11_SUBRESOURCE_DATA lightSubData{};
	lightSubData.pSysMem = &sceneLights.lights;
	lightSubData.SysMemPitch = 0;
	lightSubData.SysMemSlicePitch = 0;

	D3D11_RASTERIZER_DESC cmdesc;
	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;
	cmdesc.FrontCounterClockwise = false;

	D3D11_TEXTURE2D_DESC gameDepthStencilDesc;
	gameDepthStencilDesc.Width = screenWidth;
	gameDepthStencilDesc.Height = screenHeight;
	gameDepthStencilDesc.MipLevels = 1;
	gameDepthStencilDesc.ArraySize = 1;
	gameDepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	gameDepthStencilDesc.SampleDesc.Count = 1;
	gameDepthStencilDesc.SampleDesc.Quality = 0;
	gameDepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	gameDepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	gameDepthStencilDesc.CPUAccessFlags = 0;
	gameDepthStencilDesc.MiscFlags = 0;

	device->CreateRasterizerState(&cmdesc, cullModeState.GetAddressOf());
	device->CreateTexture2D(&gameDepthStencilDesc, nullptr, gameDepthBuffer.GetAddressOf());
	device->CreateDepthStencilView(gameDepthBuffer.Get(), nullptr, gameDepthStencil.GetAddressOf());
	device->CreateBuffer(&cbInstanceDesc, &instanceSubData, cInstanceBuffer.GetAddressOf());
	device->CreateBuffer(&cbMeshDesc, &meshSubData, cMeshBuffer.GetAddressOf());
	device->CreateBuffer(&cbSceneDesc, &sceneSubData, cSceneBuffer.GetAddressOf());
	device->CreateBuffer(&sbTransformDesc, &transformSubData, sTransformBuffer.GetAddressOf());
	device->CreateBuffer(&sbColliderDesc, &colliderSubData, sColliderBuffer.GetAddressOf());
	device->CreateBuffer(&sbBonePoseDesc, &bonePoseSubData, sBonePoseBuffer.GetAddressOf());
	device->CreateBuffer(&sbLightDesc, &lightSubData, sLightBuffer.GetAddressOf());

	device->Release();
	return true;
}
#pragma endregion

#pragma region Textures

bool MAD::DirectX11Renderer::LoadTextures()
{
	ID3D11Device* device{};
	d3d.GetDevice((void**)&device);

	PipelineHandles handles{};
	d3d.GetImmediateContext((void**)&handles);

	D3D11_SAMPLER_DESC texSampleDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());

	std::vector<std::wstring> texPaths{};
	std::wstring playerTex = L"../3DAssets/Textures/Madeline.dds";

	texPaths.push_back(playerTex);

	DirectX::CreateDDSTextureFromFile(device, texPaths[0].c_str(), nullptr, playerView.GetAddressOf());

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(handles.context);

	/*unsigned int numViewports = 1;
	handles.context->RSGetViewports(&numViewports, &uiViewport);
	viewport3D = uiViewport;*/

	device->CreateSamplerState(&texSampleDesc, texSampler.GetAddressOf());

	device->Release();
	handles.context->Release();
	return true;
}



#pragma endregion 

#pragma region SRVs
bool MAD::DirectX11Renderer::LoadShaderResources()
{
	ID3D11Device* device;
	d3d.GetDevice((void**)&device);

	D3D11_SHADER_RESOURCE_VIEW_DESC transViewDesc{};
	transViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	transViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	transViewDesc.BufferEx.FirstElement = 0;
	transViewDesc.BufferEx.NumElements = instanceMax;

	D3D11_SHADER_RESOURCE_VIEW_DESC colliderViewDesc{};
	colliderViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	colliderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	colliderViewDesc.BufferEx.FirstElement = 0;
	colliderViewDesc.BufferEx.NumElements = instanceMax;

	D3D11_SHADER_RESOURCE_VIEW_DESC bonePoseViewDesc{};
	bonePoseViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	bonePoseViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	bonePoseViewDesc.BufferEx.FirstElement = 0;
	bonePoseViewDesc.BufferEx.NumElements = 100;

	D3D11_SHADER_RESOURCE_VIEW_DESC lightViewDesc{};
	lightViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	lightViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	lightViewDesc.BufferEx.FirstElement = 0;
	lightViewDesc.BufferEx.NumElements = lightInstanceMax;

	D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc = {};
	targetBlendDesc.BlendEnable = TRUE;
	targetBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	targetBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	targetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
	targetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	targetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	targetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	targetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC alphaBlendDesc = {};
	alphaBlendDesc.AlphaToCoverageEnable = FALSE;
	alphaBlendDesc.IndependentBlendEnable = FALSE;
	alphaBlendDesc.RenderTarget[0] = targetBlendDesc;

	D3D11_TEXTURE2D_DESC gameTexDesc;
	ZeroMemory(&gameTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	gameTexDesc.Width = screenWidth;
	gameTexDesc.Height = screenHeight;
	gameTexDesc.MipLevels = 1;
	gameTexDesc.ArraySize = 1;
	gameTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gameTexDesc.SampleDesc.Count = 1;
	gameTexDesc.Usage = D3D11_USAGE_DEFAULT;
	gameTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	gameTexDesc.CPUAccessFlags = 0;
	gameTexDesc.MiscFlags = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC gameViewDesc;
	gameViewDesc.Format = gameTexDesc.Format;
	gameViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	gameViewDesc.Texture2D.MostDetailedMip = 0;
	gameViewDesc.Texture2D.MipLevels = 1;

	D3D11_RENDER_TARGET_VIEW_DESC gameTargetViewDesc;
	gameTargetViewDesc.Format = gameTexDesc.Format;
	gameTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	gameTargetViewDesc.Texture2D.MipSlice = 0;

	device->CreateShaderResourceView(sTransformBuffer.Get(),
		&transViewDesc,
		transformView.GetAddressOf());
	device->CreateShaderResourceView(sColliderBuffer.Get(), &colliderViewDesc, colliderView.GetAddressOf());
	device->CreateShaderResourceView(sBonePoseBuffer.Get(), &bonePoseViewDesc, bonePoseView.GetAddressOf());
	device->CreateShaderResourceView(sLightBuffer.Get(), nullptr, lightView.GetAddressOf());
	device->CreateTexture2D(&gameTexDesc, nullptr, targetGameTex.GetAddressOf());
	device->CreateRenderTargetView(targetGameTex.Get(), &gameTargetViewDesc, targetGameView.GetAddressOf());
	device->CreateShaderResourceView(targetGameTex.Get(), &gameViewDesc, gameView.GetAddressOf());
	device->CreateBlendState(&alphaBlendDesc, alphaBlend.GetAddressOf());

	device->Release();
	return true;
}

#pragma endregion

#pragma region Geometry
bool MAD::DirectX11Renderer::LoadGeometry()
{
	ID3D11Device* creator;
	d3d.GetDevice((void**)&creator);

	D3D11_SUBRESOURCE_DATA vertexData = { modelLoader->vertices.data(), 0, 0 };
	CD3D11_BUFFER_DESC vertexDesc(sizeof(JointVertex) * modelLoader->vertices.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&vertexDesc, &vertexData, vertexBuffer.GetAddressOf());

	D3D11_SUBRESOURCE_DATA indexData = { modelLoader->indices.data(), 0, 0 };
	CD3D11_BUFFER_DESC aiDesc(sizeof(unsigned int) * modelLoader->indices.size(), D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&aiDesc, &indexData, indexBuffer.GetAddressOf());
	creator->Release();

	D3D11_SUBRESOURCE_DATA quadVertexData = { gameScreen.verts.data(), 0, 0 };
	CD3D11_BUFFER_DESC quadVertexDesc;
	quadVertexDesc.Usage = D3D11_USAGE_DYNAMIC;
	quadVertexDesc.ByteWidth = sizeof(Vertex) * gameScreen.verts.size();
	quadVertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	quadVertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	quadVertexDesc.MiscFlags = 0;
	creator->CreateBuffer(&quadVertexDesc, &quadVertexData, quadVertexBuffer.GetAddressOf());

	D3D11_SUBRESOURCE_DATA quadIndexData = { gameScreen.indices.data(), 0, 0 };
	CD3D11_BUFFER_DESC quadIndexDesc(sizeof(unsigned int) * gameScreen.indices.size(), D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&quadIndexDesc, &quadIndexData, quadIndexBuffer.GetAddressOf());
	creator->Release();

	return true;
}
#pragma endregion

#pragma region Pipeline
bool MAD::DirectX11Renderer::SetupPipeline()
{
	ID3D11Device* device{};
	d3d.GetDevice((void**)&device);

	D3D11_INPUT_ELEMENT_DESC jointVertAttribs[5]{};

	jointVertAttribs[0].SemanticName = "POSITION";
	jointVertAttribs[0].SemanticIndex = 0;
	jointVertAttribs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	jointVertAttribs[0].InputSlot = 0;
	jointVertAttribs[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	jointVertAttribs[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	jointVertAttribs[0].InstanceDataStepRate = 0;

	jointVertAttribs[1].SemanticName = "UV";
	jointVertAttribs[1].SemanticIndex = 0;
	jointVertAttribs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	jointVertAttribs[1].InputSlot = 0;
	jointVertAttribs[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	jointVertAttribs[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	jointVertAttribs[1].InstanceDataStepRate = 0;

	jointVertAttribs[2].SemanticName = "NORM";
	jointVertAttribs[2].SemanticIndex = 0;
	jointVertAttribs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	jointVertAttribs[2].InputSlot = 0;
	jointVertAttribs[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	jointVertAttribs[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	jointVertAttribs[2].InstanceDataStepRate = 0;

	jointVertAttribs[3].SemanticName = "WEIGHTS";
	jointVertAttribs[3].SemanticIndex = 0;
	jointVertAttribs[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	jointVertAttribs[3].InputSlot = 0;
	jointVertAttribs[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	jointVertAttribs[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	jointVertAttribs[3].InstanceDataStepRate = 0;

	jointVertAttribs[4].SemanticName = "JOINTS";
	jointVertAttribs[4].SemanticIndex = 0;
	jointVertAttribs[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	jointVertAttribs[4].InputSlot = 0;
	jointVertAttribs[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	jointVertAttribs[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	jointVertAttribs[4].InstanceDataStepRate = 0;

	D3D11_INPUT_ELEMENT_DESC vertAttrib[3]{};

	vertAttrib[0].SemanticName = "POSITION";
	vertAttrib[0].SemanticIndex = 0;
	vertAttrib[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertAttrib[0].InputSlot = 0;
	vertAttrib[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertAttrib[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertAttrib[0].InstanceDataStepRate = 0;

	vertAttrib[1].SemanticName = "UV";
	vertAttrib[1].SemanticIndex = 0;
	vertAttrib[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertAttrib[1].InputSlot = 0;
	vertAttrib[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertAttrib[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertAttrib[1].InstanceDataStepRate = 0;

	vertAttrib[2].SemanticName = "NORM";
	vertAttrib[2].SemanticIndex = 0;
	vertAttrib[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertAttrib[2].InputSlot = 0;
	vertAttrib[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	vertAttrib[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	vertAttrib[2].InstanceDataStepRate = 0;

	device->CreateInputLayout(jointVertAttribs,
		ARRAYSIZE(jointVertAttribs),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		jointVertexFormat.GetAddressOf());

	device->CreateInputLayout(vertAttrib,
		ARRAYSIZE(vertAttrib),
		vsQuadBlob->GetBufferPointer(),
		vsQuadBlob->GetBufferSize(),
		vertexFormat.GetAddressOf());

	device->CreateInputLayout(nullptr,
		0,
		vsDebugBlob->GetBufferPointer(),
		vsDebugBlob->GetBufferSize(),
		debugVertexFormat.GetAddressOf());

	MAD::PipelineHandles handles{};
	d3d.GetImmediateContext((void**)&handles.context);
	d3d.GetRenderTargetView((void**)&handles.targetView);
	d3d.GetDepthStencilView((void**)&handles.depthStencil);

	ID3D11RenderTargetView* const views[] = { handles.targetView};
	handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);

	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	handles.context->IASetInputLayout(jointVertexFormat.Get());

	const UINT strides[] = { sizeof(JointVertex) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
	handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	handles.context->GSSetShader(nullptr, nullptr, 0);

	UINT startSlot = 0;
	UINT numBuffers = 3;
	ID3D11Buffer* const cBuffs[]{ cMeshBuffer.Get(), cSceneBuffer.Get(), cInstanceBuffer.Get() };
	handles.context->VSSetConstantBuffers(startSlot, numBuffers, cBuffs);
	handles.context->PSSetConstantBuffers(startSlot, numBuffers, cBuffs);

	ID3D11ShaderResourceView* vsViews[]{ transformView.Get(), bonePoseView.Get() };
	ID3D11ShaderResourceView* psViews[]{ playerView.Get(), lightView.Get() };
	handles.context->VSSetShaderResources(0, 2, vsViews);
	handles.context->PSSetShaderResources(0, 2, psViews);
	
	ID3D11SamplerState* psSamples[]{ texSampler.Get() };	
	handles.context->PSSetSamplers(0, 1, psSamples);

	handles.depthStencil->Release();
	handles.targetView->Release();
	handles.context->Release();
	device->Release();
	return true;
}

void MAD::DirectX11Renderer::SetRenderToTexPipeline(PipelineHandles& handles)
{
	const UINT strides[] = { sizeof(JointVertex) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };

	UINT startSlot = 0;
	UINT numBuffers = 3;
	ID3D11Buffer* const cBuffs[]{ cMeshBuffer.Get(), cSceneBuffer.Get(), cInstanceBuffer.Get() };
	ID3D11ShaderResourceView* vsViews[]{ transformView.Get(), bonePoseView.Get() };
	ID3D11ShaderResourceView* psViews[]{ playerView.Get(), lightView.Get() };
	ID3D11SamplerState* psSamples[]{ texSampler.Get() };

	handles.context->OMSetRenderTargets(1, targetGameView.GetAddressOf(), gameDepthStencil.Get());
	handles.context->RSSetViewports(1, &gameViewport);
	handles.context->ClearRenderTargetView(targetGameView.Get(), blue);
	handles.context->ClearDepthStencilView(gameDepthStencil.Get(), D3D11_CLEAR_DEPTH, 1, 0);
	
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	handles.context->IASetInputLayout(jointVertexFormat.Get());
	handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
	handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	handles.context->GSSetShader(nullptr, nullptr, 0);
	handles.context->VSSetConstantBuffers(startSlot, numBuffers, cBuffs);
	handles.context->PSSetConstantBuffers(startSlot, numBuffers, cBuffs);
	handles.context->VSSetShaderResources(0, 2, vsViews);
	handles.context->PSSetShaderResources(0, 2, psViews);
	handles.context->PSSetSamplers(0, 1, psSamples);

}

void MAD::DirectX11Renderer::SetRenderToQuadPipeline(PipelineHandles& handles)
{
	D3D11_MAPPED_SUBRESOURCE quadSubRes{};
	handles.context->Map(quadVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &quadSubRes);
	memcpy(quadSubRes.pData, gameScreen.verts.data(), sizeof(Vertex) * gameScreen.verts.size());
	handles.context->Unmap(quadVertexBuffer.Get(), 0);
	
	const UINT strides[] = { sizeof(Vertex) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { quadVertexBuffer.Get() };

	UINT startSlot = 0;
	UINT numBuffers = 3;
	ID3D11ShaderResourceView* psViews[]{ gameView.Get()};
	ID3D11SamplerState* psSamples[]{ texSampler.Get() };

	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(quadIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	handles.context->IASetInputLayout(vertexFormat.Get());
	handles.context->VSSetShader(quadVertexShader.Get(), nullptr, 0);
	handles.context->PSSetShader(quadPixelShader.Get(), nullptr, 0);
	handles.context->GSSetShader(nullptr, nullptr, 0);
	handles.context->PSSetShaderResources(0, 1, psViews);
	handles.context->PSSetSamplers(0, 1, psSamples);
	handles.context->RSSetState(cullModeState.Get());

}

#pragma endregion

#pragma endregion

#pragma region Drawing Logic
void MAD::DirectX11Renderer::InitRendererSystems()
{
	flecsWorld->entity("Rendering System").add<RenderingSystem>();

	startDraw = flecsWorld->system<RenderingSystem>().kind(flecs::PreUpdate)
		.each([this](flecs::entity e, RenderingSystem& s)
			{
				MAD::PipelineHandles handles{};
				d3d.GetImmediateContext((void**)&handles.context);
				d3d.GetRenderTargetView((void**)&handles.targetView);
				d3d.GetDepthStencilView((void**)&handles.depthStencil);

				handles.context->ClearRenderTargetView(handles.targetView, _black);
				handles.context->ClearDepthStencilView(handles.depthStencil, D3D11_CLEAR_DEPTH, 1, 0);

				for (int i = lightCounter; i < lightInstanceMax; i++)
				{
					sceneLights.lights[i] = {};
				}

				drawCounter = 0;
				colliderCounter = 0;
				lightCounter = 0;

				auto now = std::chrono::steady_clock::now();
				deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
				lastUpdate = now;

				handles.context->Release();
				handles.targetView->Release();
				handles.depthStencil->Release();
			});


	updateDrawMoveable = flecsWorld->system<MAD::Transform, MAD::ModelIndex, MAD::ModelOffset, MAD::RenderModel, MAD::Moveable>().kind(flecs::OnUpdate)
		.each([this](MAD::Transform& pos, MAD::ModelIndex& ndx, MAD::ModelOffset& offset, MAD::RenderModel&, MAD::Moveable&)
			{
				int i = drawCounter;

				instanceTransforms.transforms[i] = pos.value;
				GW::MATH::GVector::AddVectorF(instanceTransforms.transforms[i].row4, offset.value, instanceTransforms.transforms[i].row4);

				// increment the shared draw counter but don't go over (branchless) 
				int v = static_cast<int>(instanceMax) - static_cast<int>(drawCounter + 2);
				// if v < 0 then 0, else 1
				int sign = 1 ^ ((unsigned int)v >> (sizeof(int) * CHAR_BIT - 1));
				drawCounter += sign;

				UpdateAnimations(ndx.id, deltaTime * modelAnimSpeed);
			});

	updateDrawStatic = flecsWorld->system<MAD::Transform, MAD::ModelIndex, MAD::ModelOffset, MAD::RenderModel, MAD::StaticModel>().kind(flecs::OnUpdate)
		.each([this](MAD::Transform& pos, MAD::ModelIndex& ndx, MAD::ModelOffset& offset, MAD::RenderModel&, MAD::StaticModel&)
			{
				int i = drawCounter;
				instanceTransforms.transforms[i] = pos.value;
				GW::MATH::GVector::AddVectorF(instanceTransforms.transforms[i].row4, offset.value, instanceTransforms.transforms[i].row4);
				//instanceTransforms.modelNdxs[i] = ndx.id;

				// increment the shared draw counter but don't go over (branchless) 
				int v = static_cast<int>(instanceMax) - static_cast<int>(drawCounter + 2);
				// if v < 0 then 0, else 1
				int sign = 1 ^ ((unsigned int)v >> (sizeof(int) * CHAR_BIT - 1));
				drawCounter += sign;
			});

	updateDebug = flecsWorld->system<RenderCollider, ColliderContainer>().kind(flecs::OnUpdate)
		.each([this](RenderCollider&, ColliderContainer& colliders)
			{
				for (int i = 0; i < colliders.colliders.size(); i++)
				{
					BoxCollider* boxCollider = (BoxCollider*)colliders.colliders[i].get();
					instanceColliders.boxColliders[colliderCounter] = boxCollider->boundBox;
					colliderCounter += 1;
				}
			});

	updateLights = flecsWorld->system<PointLight, Transform>().kind(flecs::OnUpdate)
		.each([this](PointLight& pointLights, Transform& position)
			{
				int i = lightCounter;
				sceneLights.lights[i] = pointLights;
				GW::MATH::GVector::AddVectorF(sceneLights.lights[i].offset, position.value.row4, sceneLights.lights[i].offset);

				lightCounter += 1;
			});


	completeDraw = flecsWorld->system<RenderingSystem>().kind(flecs::PostUpdate)
		.each([this](flecs::entity e, RenderingSystem& s)
			{
				//Grab Pipeline Resources
				MAD::PipelineHandles handles{};
				d3d.GetImmediateContext((void**)&handles.context);
				d3d.GetRenderTargetView((void**)&handles.targetView);
				d3d.GetDepthStencilView((void**)&handles.depthStencil);

				D3D11_VIEWPORT prevViewport;
				UINT numViews = 1;
				handles.context->RSGetViewports(&numViews, &prevViewport);

				ID3D11RasterizerState* prevRasterState;
				handles.context->RSGetState(&prevRasterState);

				ID3D11RenderTargetView* const targetViews[] = { handles.targetView };

				SetRenderToTexPipeline(handles);
				Render3D(handles);

				handles.context->RSSetViewports(numViews, &prevViewport);
				handles.context->OMSetRenderTargets(1, targetViews, handles.depthStencil);

				SetRenderToQuadPipeline(handles);
				handles.context->DrawIndexed(6, 0, 0);

				handles.context->ClearRenderTargetView(targetGameView.Get(), _black);

				Render2D(handles);

				handles.depthStencil->Release();
				handles.targetView->Release();
				handles.context->Release();

			});
}

void MAD::DirectX11Renderer::Render2D(PipelineHandles& handles)
{	
	ID3D11BlendState* prevBlendState;
	ID3D11DepthStencilState* prevDepthState;
	unsigned int prevStencilRef;
	D3D11_VIEWPORT previousViewport;
	unsigned int numViewports = 1;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	ID3D11RasterizerState* prevRasterState;

	handles.context->RSGetState(&prevRasterState);
	handles.context->OMGetDepthStencilState(&prevDepthState, &prevStencilRef);
	handles.context->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);
	handles.context->RSGetViewports(&numViewports, &previousViewport);

	m_spriteBatch->SetViewport(previousViewport);

	spriteQuery.each([this](const RenderSprite&, Sprite& _sprite)
		{
			m_spriteBatch->Begin(DirectX::DX11::SpriteSortMode_Deferred, alphaBlend.Get());
			for (int i = 0; i < _sprite.props.spriteCount.numSprites; i++)
			{
				m_spriteBatch->Draw(_sprite.view.Get(),
					_sprite.props.pos.value,
					nullptr,
					DirectX::Colors::White,
					0.0f,
					_sprite.props.origin.value,
					_sprite.props.newScale.value);
			}
			m_spriteBatch->End();
		});

	textQuery.each([this](const RenderText&, Text& _txt)
		{
			m_spriteBatch->Begin(DirectX::DX11::SpriteSortMode_Deferred, alphaBlend.Get());
			_txt.props.font.type->DrawString(m_spriteBatch.get(),
				_txt.value.c_str(),
				_txt.props.newPos.value,
				_txt.props.color.value,
				0.0f,
				_txt.props.origin.value,
				_txt.props.newScale.value);
			m_spriteBatch->End();
		});

	handles.context->RSSetState(prevRasterState);
	handles.context->OMSetDepthStencilState(prevDepthState, prevStencilRef);
	handles.context->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
	handles.context->RSSetViewports(numViewports, &previousViewport);

	if (prevDepthState) prevDepthState->Release();
	if (prevBlendState) prevBlendState->Release();
}

void MAD::DirectX11Renderer::Render3D(PipelineHandles& handles)
{
	D3D11_MAPPED_SUBRESOURCE sceneSubRes{};
	D3D11_MAPPED_SUBRESOURCE instanceSubRes{};
	D3D11_MAPPED_SUBRESOURCE meshSubRes{};
	D3D11_MAPPED_SUBRESOURCE transSubRes{};
	D3D11_MAPPED_SUBRESOURCE colSubRes{};
	D3D11_MAPPED_SUBRESOURCE instSubRes{};
	D3D11_MAPPED_SUBRESOURCE mapModelSubRes{};
	D3D11_MAPPED_SUBRESOURCE poseSubRes{};
	D3D11_MAPPED_SUBRESOURCE lightSubRes{};

	handles.context->Map(cSceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sceneSubRes);
	sceneData.viewMatrix = viewMatrix;
	sceneData.projectionMatrix = projectionMatrix;
	sceneData.camPos = cameraMatrix.row4;
	memcpy(sceneSubRes.pData, &sceneData, sizeof(sceneData));
	handles.context->Unmap(cSceneBuffer.Get(), 0);

	handles.context->Map(sTransformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &transSubRes);
	memcpy(transSubRes.pData, &instanceTransforms, sizeof(TransformData) * instanceMax);
	handles.context->Unmap(sTransformBuffer.Get(), 0);

	handles.context->Map(sColliderBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &colSubRes);
	memcpy(colSubRes.pData, &instanceColliders, sizeof(GAABBMMF) * instanceMax);
	handles.context->Unmap(sColliderBuffer.Get(), 0);

	handles.context->Map(sLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &lightSubRes);
	memcpy(lightSubRes.pData, &sceneLights, sizeof(PointLight) * lightInstanceMax);
	handles.context->Unmap(sLightBuffer.Get(), 0);

	int iter = 0;

	std::string player = "Madeline.fbx";

	modelQuery.each([this, handles, &instSubRes, &meshSubRes, &iter, &poseSubRes, &player](const RenderModel&, const ModelIndex& _modelNdx, const Moveable&)
		{
			auto& model = modelLoader->models[_modelNdx.id];

			if (model.modelName.find(player) != std::string::npos)
			{
				meshData.hasTexture = 1;
			}

			handles.context->Map(sBonePoseBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &poseSubRes);
			memcpy(poseSubRes.pData, model.currPose.data(), sizeof(GW::MATH::GMATRIXF) * model.currPose.size());
			handles.context->Unmap(sBonePoseBuffer.Get(), 0);

			handles.context->Map(cInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &instSubRes);
			instanceData.transformStart = iter;
			memcpy(instSubRes.pData, &instanceData, sizeof(PerInstanceData));
			handles.context->Unmap(cInstanceBuffer.Get(), 0);

			for (int i = 0; i < model.meshes.size(); i++)
			{
				auto& mesh = model.meshes[i];
				auto& material = model.materials[mesh.materialStart];

				meshData.attribute = material.attrib;

				handles.context->Map(cMeshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &meshSubRes);
				memcpy(meshSubRes.pData, &meshData, sizeof(meshData));
				handles.context->Unmap(cMeshBuffer.Get(), 0);

				handles.context->DrawIndexedInstanced(mesh.indexCount, 1, mesh.indexStart + model.indexStart, mesh.vertexStart + model.vertexStart, 0);
			}

			iter++;
		});

	handles.context->VSSetShader(levelVertexShader.Get(), nullptr, 0);

	levelQuery.each([this, handles, &instSubRes, &meshSubRes, &iter, &poseSubRes](const RenderModel&, const ModelIndex& _modelNdx, const StaticModel&)
		{
			meshData.hasTexture = false;
			handles.context->Map(cMeshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &meshSubRes);
			memcpy(meshSubRes.pData, &meshData, sizeof(meshData));
			handles.context->Unmap(cMeshBuffer.Get(), 0);

			auto& model = modelLoader->models[_modelNdx.id];

			handles.context->Map(cInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &instSubRes);
			instanceData.transformStart = iter;
			memcpy(instSubRes.pData, &instanceData, sizeof(PerInstanceData));
			handles.context->Unmap(cInstanceBuffer.Get(), 0);

			for (int i = 0; i < model.meshes.size(); i++)
			{
				auto& mesh = model.meshes[i];
				auto& material = model.materials[mesh.materialStart];

				meshData.attribute = material.attrib;
				meshData.hasTexture = 0;
				handles.context->Map(cMeshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &meshSubRes);
				memcpy(meshSubRes.pData, &meshData, sizeof(meshData));
				handles.context->Unmap(cMeshBuffer.Get(), 0);

				handles.context->DrawIndexedInstanced(mesh.indexCount, 1, mesh.indexStart + model.indexStart, mesh.vertexStart + model.vertexStart, 0);
			}

			iter++;

		});

	if (isDebugOn)
	{
		SetDebugPipeline(handles);

		handles.context->DrawInstanced(1, colliderCounter, 0, 0);
	}
}

void MAD::DirectX11Renderer::Restore3DStates(PipelineHandles& handles)
{
	const UINT strides[] = { sizeof(JointVertex) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };

	UINT startSlot = 0;
	UINT numBuffers = 3;
	ID3D11Buffer* const cBuffs[]{ cMeshBuffer.Get(), cSceneBuffer.Get(), cInstanceBuffer.Get() };
	ID3D11ShaderResourceView* vsViews[]{ transformView.Get(), bonePoseView.Get() };
	ID3D11ShaderResourceView* psViews[]{ playerView.Get(), lightView.Get() };
	ID3D11SamplerState* psSamples[]{ texSampler.Get() };

	//handles.context->RSSetViewports(1, &viewport3D);
	handles.context->PSSetSamplers(0, 1, psSamples);
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	handles.context->IASetInputLayout(jointVertexFormat.Get());
	handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
	handles.context->GSSetShader(nullptr, nullptr, 0);
	handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	handles.context->VSSetConstantBuffers(startSlot, numBuffers, cBuffs);
	handles.context->PSSetConstantBuffers(startSlot, numBuffers, cBuffs);
	handles.context->VSSetShaderResources(0, 2, vsViews);
	handles.context->PSSetShaderResources(0, 2, psViews);
}

void MAD::DirectX11Renderer::SetDebugPipeline(PipelineHandles& handles)
{
	ID3D11ShaderResourceView* gsViews[]{ colliderView.Get() };
	ID3D11Buffer* const geoCBuffs[]{ cSceneBuffer.Get(), cInstanceBuffer.Get() };

	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	handles.context->IASetInputLayout(debugVertexFormat.Get());
	handles.context->VSSetShader(debugVertexShader.Get(), nullptr, 0);
	handles.context->GSSetShader(geometryShader.Get(), nullptr, 0);
	handles.context->PSSetShader(debugPixelShader.Get(), nullptr, 0);
	handles.context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	handles.context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, NULL);
	handles.context->GSSetConstantBuffers(0, 2, geoCBuffs);
	handles.context->GSSetShaderResources(0, 1, gsViews);
}

#pragma endregion

#pragma region Flecs Systems
bool MAD::DirectX11Renderer::Activate(bool _runSystems)
{
	if (startDraw.is_alive() && updateDrawMoveable.is_alive() && updateDrawStatic.is_alive() && updateDebug.is_alive() && completeDraw.is_alive() && updateLights.is_alive())
	{
		if (_runSystems)
		{
			startDraw.enable();
			updateDrawMoveable.enable();
			updateDrawStatic.enable();
			updateDebug.enable();
			updateLights.enable();
			completeDraw.enable();

		}
		else
		{
			startDraw.disable();
			updateDrawMoveable.disable();
			updateDrawStatic.disable();
			updateDebug.disable();
			updateLights.disable();
			completeDraw.disable();
		}

		return true;
	}

	return false;
}

bool MAD::DirectX11Renderer::Shutdown()
{
	startDraw.destruct();
	updateDrawMoveable.destruct();
	updateDrawStatic.destruct();
	updateDebug.destruct();
	completeDraw.destruct();
	bombEffect = nullptr;
	flecsWorld->entity("QueryPlayerTransforms").destruct();

	m_spriteBatch.reset();
	return true;
}

#pragma endregion

#pragma region Update
void MAD::DirectX11Renderer::UpdateAnimations(unsigned modelNdx, float deltaTime)
{
	if (!modelAnimPause)
	{
		modelElapsedTime += deltaTime;

		if (animationState.isBlending)
		{
			animationState.transitionTimer += deltaTime;
			UpdateAnimationsBlended(modelNdx, modelElapsedTime, animationState.transitionTimer / transitionLength);
		}
		else
		{
			if (modelLoader->models[modelNdx].model->HasAnimations())
				modelLoader->models[modelNdx].UpdatePose(modelElapsedTime, animationState.currentAnimNDX);
		}
	}
}

void MAD::DirectX11Renderer::UpdateAnimationsBlended(unsigned modelNdx, float _modelElapsedTime, float transitionTime)
{
	if (modelLoader->models[modelNdx].model->HasAnimations())
		modelLoader->models[modelNdx].UpdatePoseBlended(_modelElapsedTime, animationState.currentAnimNDX, animationState.nextAnimNdx, transitionTime);

	if (animationState.transitionTimer >= transitionLength)
	{
		animationState.transitionTimer = 0.0f;
		animationState.isBlending = false;
		animationState.currentAnimNDX = animationState.nextAnimNdx;
	}
}

void MAD::DirectX11Renderer::UpdateProjectionMatrix(float newAspect)
{
	GW::MATH::GMatrix::ProjectionDirectXLHF(fov, newAspect, nearPlane, farPlane, projectionMatrix);
}

//Quad MAD::DirectX11Renderer::MakeQuad(float x, float y, float width, float height, DirectX::XMFLOAT4 color)
//{
//	Quad newQuad = {};
//
//	newQuad.topLeft = { DirectX::XMFLOAT3(x, y, 0.0f), color };					//topleft
//	newQuad.topRight = { DirectX::XMFLOAT3(x + width, y, 0.0f), color };			//topright
//	newQuad.bottomLeft = { DirectX::XMFLOAT3(x, y + height, 0.0f), color };			//bottomleft
//	newQuad.bottomRight = { DirectX::XMFLOAT3(x + width, y + height, 0.0f), color };	//bottomright
//
//	return newQuad;
//}

void MAD::DirectX11Renderer::UpdateCamera()
{
	float deltaTime;
	auto currTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> _deltaTime = currTime - prevTime;
	prevTime = currTime;
	deltaTime = _deltaTime.count();

	const float camSpeed = 2.0f;
	float deltaX;
	float deltaY;
	float deltaZ;

	float upValue;
	float downValue;
	float forwardValue;
	float backValue;
	float leftValue;
	float rightValue;

	float mouseX;
	float mouseY;

	unsigned int windowHeight;
	window.GetHeight(windowHeight);

	unsigned int windowWidth;
	window.GetWidth(windowWidth);

	float aspect;
	d3d.GetAspectRatio(aspect);

	float fov = 65.0f * 3.14f / 180.0f;
	float pitch;
	float yaw;

	GW::GReturn mouseCheck;

	input.GetState(G_KEY_SPACE, upValue);
	input.GetState(G_KEY_LEFTSHIFT, downValue);
	input.GetState(G_KEY_W, forwardValue);
	input.GetState(G_KEY_A, leftValue);
	input.GetState(G_KEY_S, backValue);
	input.GetState(G_KEY_D, rightValue);
	mouseCheck = input.GetMouseDelta(mouseX, mouseY);

	if (mouseCheck == GW::GReturn::REDUNDANT)
	{
		mouseX = 0;
		mouseY = 0;
	}

	GW::MATH::GMatrix::InverseF(viewMatrix, viewMatrix);
	cameraMatrix = viewMatrix;

	deltaX = rightValue - leftValue;
	deltaY = upValue - downValue;
	deltaZ = forwardValue - backValue;

	pitch = fov * mouseY / windowHeight;
	yaw = fov * aspect * mouseX / windowWidth;

	GW::MATH::GVECTORF yTrans = { 0.0f, (deltaY * camSpeed * deltaTime), 0.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(cameraMatrix, yTrans, cameraMatrix);

	GW::MATH::GVECTORF transXZ = { (deltaX * camSpeed * deltaTime), 0, (deltaZ * camSpeed * deltaTime) };
	GW::MATH::GMatrix::TranslateLocalF(cameraMatrix, transXZ, cameraMatrix);

	GW::MATH::GMatrix::RotateXLocalF(cameraMatrix, pitch, cameraMatrix);
	GW::MATH::GMatrix::RotateYGlobalF(cameraMatrix, yaw, cameraMatrix);

	GW::MATH::GMatrix::InverseF(cameraMatrix, viewMatrix);
}



void MAD::DirectX11Renderer::UpdateCamera(GW::MATH::GMATRIXF camWorld)
{
	cameraMatrix = camWorld;
	GW::MATH::GMatrix::InverseF(camWorld, viewMatrix);
}

//void MAD::DirectX11Renderer::SetViewport(D3D11_VIEWPORT& _viewport)
//{
//	viewport3D = _viewport;
//}

//void MAD::DirectX11Renderer::UpdateBorders(float barWidth, float barHeight, float newWidth, float newHeight, float viewportWidth, DirectX::XMFLOAT4 color)
//{
//	screenBorderQuads.clear();
//	screenBorderVerts.clear();
//
//	Quad left = MakeQuad(0.0f, 0.0f, barWidth, newHeight, color);
//	Quad right = MakeQuad(newWidth - barWidth, 0.0f, barWidth, newHeight, color);
//	Quad top = MakeQuad(barWidth, 0.0f, viewportWidth, barHeight, color);
//	Quad bottom = MakeQuad(barWidth, newHeight - barHeight, viewportWidth, barHeight, color);
//
//	screenBorderQuads.push_back(left);
//	screenBorderQuads.push_back(right);
//	screenBorderQuads.push_back(top);
//	screenBorderQuads.push_back(bottom);
//
//	for (int i = 0; i < screenBorderQuads.size(); i++)
//	{
//		screenBorderVerts.push_back(screenBorderQuads[i].topLeft);
//		screenBorderVerts.push_back(screenBorderQuads[i].topRight);
//		screenBorderVerts.push_back(screenBorderQuads[i].bottomLeft);
//		screenBorderVerts.push_back(screenBorderQuads[i].bottomRight);
//	}
//}

#pragma endregion

#pragma region Helper Functions
void MAD::DirectX11Renderer::ScreenToWorldSpace(float x, float y, GW::MATH::GVECTORF& outPoint)
{
	unsigned screenWidth;
	unsigned screenHeight;
	window.GetClientWidth(screenWidth);
	window.GetClientHeight(screenHeight);

	DirectX::XMMATRIX xmProjectionMatrix = DirectX::XMMATRIX(projectionMatrix.data);
	DirectX::XMMATRIX xmViewMatrix = DirectX::XMMATRIX(viewMatrix.data);

	DirectX::XMVECTOR screenPosNear = { x, y, 0, 1 };
	DirectX::XMVECTOR screenPosFar = { x, y, 1, 1 };

	DirectX::XMVECTOR xmWorldPosNear = DirectX::XMVector3Unproject(
		screenPosNear,
		0, 0,
		screenWidth,
		screenHeight,
		0, 1,
		xmProjectionMatrix,
		xmViewMatrix,
		DirectX::XMMatrixIdentity());

	DirectX::XMVECTOR xmWorldPosFar = DirectX::XMVector3Unproject(
		screenPosFar,
		0, 0,
		screenWidth,
		screenHeight,
		0, 1,
		xmProjectionMatrix,
		xmViewMatrix,
		DirectX::XMMatrixIdentity());

	GW::MATH::GVECTORF worldNear = {
		xmWorldPosNear.m128_f32[0],
		xmWorldPosNear.m128_f32[1],
		xmWorldPosNear.m128_f32[2],
		xmWorldPosNear.m128_f32[3],
	};

	GW::MATH::GVECTORF worldFar = {
		xmWorldPosFar.m128_f32[0],
		xmWorldPosFar.m128_f32[1],
		xmWorldPosFar.m128_f32[2],
		xmWorldPosFar.m128_f32[3],
	};

	float ratio = -worldNear.z / (worldFar.z - worldNear.z);
	GW::MATH::GVector::LerpF(worldNear, worldFar, ratio, outPoint);
}

std::string MAD::DirectX11Renderer::ReadFileIntoString(const char* _filePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file;

	file.Create();
	file.GetFileSize(_filePath, stringLength);

	if (stringLength > 0 && +file.OpenBinaryRead(_filePath))
	{
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: File \"" << _filePath << "\" Not Found!" << std::endl;

	return output;
}

void MAD::DirectX11Renderer::PrintLabeledDebugString(const char* _label, const char* _toPrint)
{
	std::cout << _label << _toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(_label);
	OutputDebugStringA(_toPrint);
#endif
}
#pragma endregion