#ifndef RENDERER_H
#define RENDERER_H

#include <d3dcompiler.h> // required for compiling shaders on the fly, consider pre-compiling instead
#pragma comment(lib, "d3dcompiler.lib")
#include "../GameConfig.h"
#include "../Events/Playevents.h"
#include "../Loaders/ModelLoader.h"
#include "../Loaders/SpriteLoader.h"
#include <DDSTextureLoader.h>
#include <SimpleMath.h>
#include <WICTextureLoader.h>
#include <PostProcess.h>
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "../Components/UI.h"
#include "../Components/Lights.h"
#include "../Components/Tiles.h"
#include "../Events/GameStateEvents.h"
#include "../Utils/PrimitiveShapes.h"

namespace MAD
{
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};

	struct TextureData
	{
		unsigned width;
		unsigned height;
	};

	struct AnimationState
	{
		unsigned currentAnimNDX;
		unsigned nextAnimNdx;
		float transitionTimer;
		bool isBlending;
	};

	struct TransformData
	{
		GW::MATH::GMATRIXF transform;
	};

	struct alignas(16) PerInstanceData
	{
		unsigned int transformStart;
		unsigned int materialStart;
		unsigned int colliderStart;
		unsigned int colliderEnd;
		
	};

	struct alignas(16) SceneData
	{
		GW::MATH::GMATRIXF viewMatrix;
		GW::MATH::GMATRIXF projectionMatrix;
		GW::MATH::GVECTORF camPos;
		GW::MATH::GVECTORF dirLightDir, dirLightColor;
		GW::MATH::GVECTORF ambientTerm;
		GW::MATH::GVECTORF fogColor;
		float fogDensity;
		float fogStartDistance;
		float contrast;
		float saturation;
	};

	struct alignas(16) MeshData
	{
		GW::MATH::GMATRIXF worldMatrix;
		Attributes attribute;
		unsigned hasTexture;
	};

	struct RenderingSystem {};

	class DirectX11Renderer
	{
		std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

		//---------- Flecs ----------
		std::shared_ptr<flecs::world> flecsWorld;
		std::shared_ptr<flecs::world> uiWorld;

		flecs::system startDraw;
		flecs::system updateDrawMoveable;
		flecs::system updateDrawStatic;
		flecs::system updateDebug;
		flecs::system updateLights;
		flecs::system completeDraw;

		std::weak_ptr<const GameConfig> gameConfig;
		std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
		flecs::query<Player, Transform> playerTransformsQuery;
		flecs::query<const RenderModel, const ModelIndex, const Moveable> modelQuery;
		flecs::query<const RenderModel, const AnimateModel, const Moveable, const ModelIndex> animationQuery;
		flecs::query<const RenderModel, const ModelIndex, const StaticModel> levelQuery;
		flecs::query<const RenderSprite, Sprite> spriteQuery;
		flecs::query<const RenderText, Text> textQuery;

		//----------Gateware----------	
		GW::SYSTEM::GWindow window;
		GW::GRAPHICS::GDirectX11Surface d3d;
		GW::MATH::GVECTORF bgColor;
		GW::INPUT::GInput input;

		//----------Shaders----------
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> levelVertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> debugVertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> quadVertexShader;
		
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> quadPixelShader;
		//Microsoft::WRL::ComPtr<ID3D11PixelShader> mapModelsPixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> debugPixelShader;

		Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShader;

		//----------Blobs----------
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> vsLevelBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> vsDebugBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> vsQuadBlob;

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> psQuadBlob;
		//Microsoft::WRL::ComPtr<ID3DBlob> psMapModelsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> psDebugBlob;

		Microsoft::WRL::ComPtr<ID3DBlob> gsBlob;

		Microsoft::WRL::ComPtr<ID3DBlob> errors;
	
		//----------Input Layouts----------
		Microsoft::WRL::ComPtr<ID3D11InputLayout> vertexFormat;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> jointVertexFormat;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> debugVertexFormat;

		//----------Geometry----------
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> quadVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> quadIndexBuffer;

		//----------Constant Buffers----------
		Microsoft::WRL::ComPtr<ID3D11Buffer> cMeshBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> cSceneBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> cInstanceBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> cMapModelBuffer;

		//----------Structured Buffers----------
		Microsoft::WRL::ComPtr<ID3D11Buffer> sTransformBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> transformView;
		Microsoft::WRL::ComPtr<ID3D11Buffer> sLightBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lightView;
		Microsoft::WRL::ComPtr<ID3D11Buffer> sColliderBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colliderView;
		Microsoft::WRL::ComPtr<ID3D11Buffer> sBonePoseBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bonePoseView;

		//----------Textures----------
		Microsoft::WRL::ComPtr<ID3D11SamplerState> texSampler;
		Microsoft::WRL::ComPtr<ID3D11BlendState> alphaBlend;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> playerView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> gameDepthBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> targetGameTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gameView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> mapDepthBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> targetTexMap;
		unsigned int texId;

		//----------ViewPorts----------
		D3D11_VIEWPORT uiViewport;
		D3D11_VIEWPORT gameViewport;
		D3D11_VIEWPORT originalViewport;

		//----------Depth Stencils----------
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> gameDepthStencil;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mapDepthStencil;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> origDepthState;
		unsigned int origStencilRef;

		//-----------Render Target Views----------
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> originalTargetView;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetGameView;

		//----------Rasterizer States----------
		ID3D11RasterizerState* originalState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> cullModeState;

		//----------Minimap----------
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetViewMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mapView;
		
		
		unsigned int playerCurrScore;
		
		//----------Level----------
		std::shared_ptr<ModelLoader> modelLoader;
			
		PerInstanceData instanceData;
		MeshData meshData;
		SceneData sceneData;

		GW::MATH::GMATRIXF modelScalar;

		Attributes modelAttribute;

		GW::MATH::GMATRIXF worldMatrix;		
		
		GW::MATH::GMATRIXF projectionMatrix;
		GW::MATH::GMATRIXF cameraMatrix;
		GW::MATH::GVECTORF camPos;
		float fov;
		float aspect;
		float nearPlane;
		float farPlane;

		GW::SYSTEM::GDaemon bombEffect;
		unsigned int updateBombEffectTime = 1;
		unsigned int bombEffectStartTime;
		unsigned int bombEffectTime = 500;

		//----------Background----------
		GW::MATH::GVECTORF black;
		GW::MATH::GVECTORF white;
		FLOAT blue[4] = {0.7f, 0.7f, 0.85f, 1.0f};
		FLOAT _black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		GW::MATH::GVECTORF rust;
		bool isStateChanged;

		GW::MATH::GVECTORF currentBgColorData;
		std::vector<GW::MATH::GVECTORF> bgColorData;
		
		DirectX::XMVECTOR creditsPosCopy;

		float screenWidth;
		float screenHeight;
		
	public:
		float deltaTime;
		GW::MATH::GMATRIXF viewMatrix;
		bool isDebugOn;	
		float uiScalar;
		bool modelAnimPause;
		float modelElapsedTime;
		float modelAnimSpeed;
		float transitionLength;
		AnimationState animationState;
		Quad gameScreen;

		bool Init(	GW::SYSTEM::GWindow _win, 
					GW::GRAPHICS::GDirectX11Surface _d3d,
					std::shared_ptr<flecs::world> _game,
					std::shared_ptr<flecs::world> _uiWorld,
					std::weak_ptr<const GameConfig> _gameConfig, std::shared_ptr<ModelLoader> _models);
		void UpdateCamera();
		void UpdateCamera(GW::MATH::GMATRIXF camWorld);	
		void UpdateAnimations(unsigned modelNdx, float deltaTime);
		void UpdateAnimationsBlended(unsigned modelNdx, float _modelElaspedTime, float transitionTime);
		void InitRendererSystems();
		bool Activate(bool runSystem);
		bool Shutdown();
		void UpdateProjectionMatrix(float newAspect);
		void ScreenToWorldSpace(float x, float y, GW::MATH::GVECTORF& outPoint);

	private:		
		std::chrono::steady_clock::time_point lastUpdate;	
		bool LoadShaders();
		bool LoadBuffers();
		bool LoadGeometry();
		bool LoadShaderResources();
		bool LoadTextures();
		void Render2D(PipelineHandles& handles);
		void Render3D(PipelineHandles& handles);
		bool SetupPipeline();
		void SetRenderToTexPipeline(PipelineHandles& handles);
		void SetRenderToQuadPipeline(PipelineHandles& handles);
		void Restore3DStates(PipelineHandles& handles);	
		void SetDebugPipeline(PipelineHandles& handles);
		std::string ReadFileIntoString(const char* _filePath);
		void PrintLabeledDebugString(const char* _label, const char* _toPrint);
		
		static constexpr unsigned int instanceMax = 2048;
		
		static constexpr unsigned int lightInstanceMax = 32;
		struct INSTANCE_TRANSFORMS
		{
			GW::MATH::GMATRIXF transforms[instanceMax];

		}instanceTransforms;

		struct INSTANCE_OBBS
		{
			GAABBMMF boxColliders[instanceMax];

		}instanceColliders;

		int drawCounter = 0;
		int colliderCounter = 0;
		
		struct INSTANCE_POSE
		{
			std::vector<GW::MATH::GMATRIXF> pose;

		}instancePose;

		int lightCounter = 0;
		struct SCENE_LIGHTS
		{
			PointLight lights[lightInstanceMax];

		}sceneLights;
	};
}

#endif