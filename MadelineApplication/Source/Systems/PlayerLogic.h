// The player system is responsible for allowing control over the main ship(s)
#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"

#include "../Loaders/SaveLoader.h"

#include "../Components/AudioSource.h"
#include "../Components/Gameplay.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/HapticSource.h"

#include "../Components/Gameplay.h"

#include "../Events/Playevents.h"
#include "../Events/LevelEvents.h"
#include "../Events/AnimationEvents.h"
#include "../Events/CameraEvents.h"
#include "../Events/TouchEvents.h"

// example space game (avoid name collisions)
namespace MAD
{
	class PlayerLogic
	{
		enum ControlState { NORMAL, DASHING, CLIMBING };
		enum WallSide { LEFT, RIGHT };

	private:
		std::shared_ptr<flecs::world> flecsWorld;
		flecs::world flecsWorldAsync;
		GW::CORE::GThreadShared flecsWorldLock;
		GW::SYSTEM::GWindow window;
		std::weak_ptr<const GameConfig> gameConfig;

		GW::INPUT::GInput keyboardMouseInput;
		GW::INPUT::GController gamePadInput;

		GW::CORE::GEventGenerator playEventPusher;
		GW::CORE::GEventGenerator cameraEventPusher;
		GW::CORE::GEventGenerator levelEventPusher;
		GW::CORE::GEventGenerator animEventPusher;
		GW::CORE::GEventGenerator touchEventPusher;
		GW::CORE::GEventResponder playEventHandler;
		GW::CORE::GEventResponder levelEventHandler;

		std::shared_ptr<SaveLoader> saveLoader;

		flecs::system playerControllerSystem;

		flecs::query<const Player, const Transform> playerQuery;


		// Running
		ControlState controlState;

		float lastXAxis = -1;
		float lastYAxis = 0;

		float runAcclTime;
		float runDcclTime;
		float maxRunSpeed;
		float minRunSpeed;

		UINT8 isFacingRight : 1;

		// Jumping
		float gravityScale;
		float jumpGravityScale;
		float stopJumpGravityScale;
		float wallSlideGravityScale;
		float springJumpGravityScale;

		float jumpSpeed;
		float wallJumpHSpeed;
		float springJumpSpeed;
		float maxYSpeed;
		float minWallSlideSpeed;
		float maxWallSlideSpeed;
		float wallSlideSpeedMult;

		float maxWallJumpTime;
		float wallJumpTime = 0;
		float maxJumpBufferTime;
		float maxJumpQueueTime;
		float jumpQueueTime = 0;

		float timeSinceGrounded;

		float springJumpCamShakeDist;
		float springJumpCamShakeTime;

		UINT8 isGrounded : 1;
		UINT8 isJumping : 1;
		UINT8 isWallJumping : 1;
		UINT8 isSpringJumping : 1;
		UINT8 isDashJumping : 1;
		UINT8 hasJumped : 1;
		UINT8 isJumpQueued : 1;
		UINT8 isJumpBuffered : 1;

		// Dashing
		int maxDashCount;
		int dashCount;

		float dashSpeed;
		float minDashSpeed;
		float dashDcclTime;
		float maxDashTime;
		float dashSlowTime;
		float dashSlowTimeScale;
		float dashTime = 0;

		float dashCamShakeDist;
		float dashCamShakeTime;

		GVECTORF dashDir;

		UINT8 isDashDccling : 1;

		// Climbing
		float climbMaxStamina;
		float climbStamina;
		float climbUpCost;
		float climbStillCost;
		float climbJumpCost;

		float maxClimbUpSpeed;
		float maxClimbSlipSpeed;
		float maxClimbDownSpeed;
		float minClimbSpeed;
		float climbAcclTime;
		float climbSlipAcclTime;
		float climbDcclTime;

		float climbVaultSpeed;
		float climbVaultGravity;
		float climbVaultHSpeed;
		float maxClimbVaultTime;
		float climbVaultTime = 0;
		float climbVaultDir;

		UINT8 isTouchingRightWall : 1;
		UINT8 isTouchingLeftWall : 1;
		UINT8 isClimbingRightWall : 1;
		UINT8 isWallSliding : 1;
		UINT8 isClimbVaulting : 1;

		// Scene
		float outOfBoundsRange;
		float exitSpeed;
		UINT8 isExitingScene : 1;

		// Interactables
		UINT8 hitSpring : 1;
		UINT8 hitStrawberry : 1;

		std::vector<flecs::id> groundObjectsTouching;
		std::vector<flecs::id> leftObjectsTouching;
		std::vector<flecs::id> rightObjectsTouching;

		// Death
		UINT8 killPlayer : 1;
		float deathCamShakeDist;
		float deathCamShakeTime;

		// Triggers
		int groundTriggerId = 0;
		int rightTriggerId = 1;
		int	leftTriggerId = 2;

		// Animation
		ANIM_EVENT curAnimEvent;
		float curAnimSpeed;

		// Input
		UINT8 isJumpPressed : 1;
		UINT8 isDashPressed : 1;

	public:
#pragma region Init
		bool Init(
			std::shared_ptr<flecs::world> _game,
			GW::SYSTEM::GWindow _window,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::INPUT::GInput _keyboardMouseInput,
			GW::INPUT::GController _gamePadInput,
			GW::CORE::GEventGenerator _playEventPusher,
			GW::CORE::GEventGenerator _cameraEventPusher,
			GW::CORE::GEventGenerator _levelEventPusher,
			GW::CORE::GEventGenerator _animEventPusher,
			GW::CORE::GEventGenerator _touchEventPusher,
			std::shared_ptr<SaveLoader> _saveLoader);
#pragma endregion

	private:
#pragma region INI
		void LoadINIStats();
#pragma endregion

#pragma region Event Responders
		void InitEventResponders();
#pragma endregion

#pragma region Init Systems
		void InitPlayerControlSystem();
#pragma endregion

#pragma region Event Pushers
		void PushPlayEvent(PlayEvent _event, PLAY_EVENT_DATA _data);
		void PushCameraEvent(CameraEvent _event, CameraEventData _data);
		void PushAnimationEvent(ANIM_EVENT event, float _animSpeed);
		void PushTouchEvent(TouchEvent _event, TouchEventData _data);
#pragma endregion

#pragma region Play Events
		void OnHapticsActivated(PLAY_EVENT_DATA _data);
		void OnHitSpring(PLAY_EVENT_DATA _data);
		void OnHitStrawberry(PLAY_EVENT_DATA _data);
		void OnHitCrystal(PLAY_EVENT_DATA _data);
		void OnHitSpikes(PLAY_EVENT_DATA _data);
		void OnKillPlayer(PLAY_EVENT_DATA _data);
#pragma endregion

#pragma region Level Events
		void OnEnterSceneBegin(LEVEL_EVENT_DATA _data);
		void OnEnterSceneDone(LEVEL_EVENT_DATA _data);
#pragma endregion


#pragma region Moving
		void HandleMovementInput(
			float _xAxis,
			float _yAxis,
			flecs::entity _entity,
			Acceleration& _acceleration,
			Velocity& _velocity,
			Transform& _transform,
			FlipInfo& _flipInfo);

		void FlipEntity(float _deltaTime, float _dirMoving, Transform& _transform, FlipInfo& _flipInfo);
#pragma endregion

#pragma region Jumping
		void HandleJumpInput(
			float _jumpInput,
			float _xAxis,
			flecs::entity _entity,
			Acceleration& _acceleration,
			Velocity& _velocity);

		void Jump(Velocity& _velocity);

		void WallJump(Acceleration& _acceleration, Velocity& _velocity, float dir);

		void SpringJump(Acceleration& _acceleration, Velocity& _velocity);

		bool CanWallJump();

		void StartJumpQueue();

		bool CanBufferJump();

		bool CanQueueJump();

		bool canQueueWallJump();

		void HitGround(
			Acceleration& _acceleration,
			Velocity& _velocity);

		void LeaveGround();
#pragma endregion

#pragma region Dashing
		void HandleDashInput(
			float _dashInput,
			float _xAxis,
			float _yAxis,
			flecs::entity _entity,
			Acceleration& _acceleration,
			Velocity& _velocity
		);

		void Dash(
			Velocity& _velocity,
			Acceleration& _acceleration,
			float _xAxis,
			float _yAxis);

		void StopDash();
#pragma endregion

#pragma region Climbing
		void HandleClimbInput(
			float _climbInput,
			float _jumpInput,
			float _xAxis,
			float _yAxis,
			flecs::entity _entity,
			Acceleration& _acceleration,
			Velocity& _velocity
		);

		bool CanStartClimb(float _xAxis);

		void StartClimb(Acceleration& _acceleration, Velocity& _velocity, float _xAxis);
		void StopClimb();
		void FallOffClimb(Acceleration& _acceleration, Velocity& _velocity, float _dir);

		void ClimbVault(Acceleration& _acceleration, Velocity& _velocity, float _dir);

		void StartWallSlide(Velocity& _velocity);
		void StopWallSlide();

		void HitWall(WallSide _wallSide);
		void LeaveWall(WallSide _wallSide, Acceleration& _acceleration, Velocity& _velocity);
#pragma endregion


#pragma region Triggers
		void HandleTriggers(ColliderContainer& _colliderContainer,
			Acceleration& _acceleration,
			Velocity& _velocity);

		void UpdateGroundObjectsTouching(std::vector<Collider*>& _colliders);
		void UpdateLeftObjectsTouching(std::vector<Collider*>& _colliders);
		void UpdateRightObjectsTouching(std::vector<Collider*>& _colliders);
		void UpdateObjectsTouching(std::vector<flecs::id>& _touchingVector, std::vector<Collider*>& _colliders);
		void GetEnteredCollisions(
			std::vector<flecs::id>& _touchingVector, 
			std::vector<Collider*>& _colliders, 
			std::vector<flecs::id>& _outIds);
		void GetExitedCollisions(
			std::vector<flecs::id>& _touchingVector,
			std::vector<Collider*>& _colliders,
			std::vector<flecs::id>& _outIds);
#pragma endregion

#pragma region Death
		void HandleOutOfBounds(flecs::entity _entity, Transform& _transform);
		void Die();
#pragma endregion

#pragma region Stats
		void ReplenishStats();

		bool CanReplenishStats();
#pragma endregion

#pragma region Haptics
		void ActivateHaptics(HapticType _type);
#pragma endregion

#pragma region Animation
		void PlayAnimation(ANIM_EVENT _animation, float _animSpeed);
#pragma endregion

#pragma region Effects
		void ShakeCamera(GVECTORF _dir, float _distance, float _time);
#pragma endregion

	public:
#pragma region Shutdown / Activate
		void GameplayStop();

		bool Activate(bool runSystem);

		bool Shutdown();
#pragma endregion
	};

};

#endif