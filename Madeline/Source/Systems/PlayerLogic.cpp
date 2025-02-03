#include "PlayerLogic.h"

#include "../Entities/Prefabs.h"

#include "../Components/Visuals.h"

#include "../Utils/Macros.h"

using namespace MAD;
using namespace flecs;
using namespace GW;
using namespace INPUT;
using namespace AUDIO;
using namespace CORE;
using namespace MATH;
using namespace MATH2D;

#pragma region Init
bool PlayerLogic::Init(
	std::shared_ptr<world> _flecsWorld,
	GW::SYSTEM::GWindow _window,
	std::weak_ptr<const GameConfig> _gameConfig,
	GInput _keyboardMouseInput,
	GController _gamePadInput,
	GEventGenerator _playEventPusher,
	GEventGenerator _cameraEventPusher,
	GEventGenerator _levelEventPusher,
	GEventGenerator _animEventPusher,
	GEventGenerator _touchEventPusher,
	std::shared_ptr<SaveLoader> _saveLoader)
{
	flecsWorld = _flecsWorld;
	window = _window;
	gameConfig = _gameConfig;
	keyboardMouseInput = _keyboardMouseInput;
	gamePadInput = _gamePadInput;
	playEventPusher = _playEventPusher;
	cameraEventPusher = _cameraEventPusher;
	levelEventPusher = _levelEventPusher;
	animEventPusher = _animEventPusher;
	touchEventPusher = _touchEventPusher;

	saveLoader = _saveLoader;

	isGrounded = false;
	killPlayer = false;
	controlState = ControlState::NORMAL;

	playerQuery = flecsWorld->query<const Player, const Transform>();

	LoadINIStats();
	InitEventResponders();
	InitPlayerControlSystem();

	return true;
}
#pragma endregion

#pragma region INI
void MAD::PlayerLogic::LoadINIStats()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	// Running
	runAcclTime = readCfg->at("PlayerStats").at("runAcclTime").as<float>();
	runDcclTime = readCfg->at("PlayerStats").at("runDcclTime").as<float>();
	maxRunSpeed = readCfg->at("PlayerStats").at("maxRunSpeed").as<float>();
	minRunSpeed = readCfg->at("PlayerStats").at("minRunSpeed").as<float>();

	// Jumping
	gravityScale = readCfg->at("PlayerStats").at("gravityScale").as<float>();
	jumpGravityScale = readCfg->at("PlayerStats").at("jumpGravityScale").as<float>();
	stopJumpGravityScale = readCfg->at("PlayerStats").at("stopJumpGravityScale").as<float>();
	wallSlideGravityScale = readCfg->at("PlayerStats").at("wallSlideGravityScale").as<float>();
	springJumpGravityScale = readCfg->at("PlayerStats").at("springJumpGravityScale").as<float>();

	jumpSpeed = readCfg->at("PlayerStats").at("jumpSpeed").as<float>();
	wallJumpHSpeed = readCfg->at("PlayerStats").at("wallJumpHSpeed").as<float>();
	springJumpSpeed = readCfg->at("PlayerStats").at("springJumpSpeed").as<float>();
	maxYSpeed = readCfg->at("PlayerStats").at("maxYSpeed").as<float>();
	minWallSlideSpeed = readCfg->at("PlayerStats").at("minWallSlideSpeed").as<float>();
	maxWallSlideSpeed = readCfg->at("PlayerStats").at("maxWallSlideSpeed").as<float>();
	wallSlideSpeedMult = readCfg->at("PlayerStats").at("wallSlideSpeedMult").as<float>();

	maxWallJumpTime = readCfg->at("PlayerStats").at("maxWallJumpTime").as<float>();
	maxJumpBufferTime = readCfg->at("PlayerStats").at("maxJumpBufferTime").as<float>();
	maxJumpQueueTime = readCfg->at("PlayerStats").at("maxJumpQueueTime").as<float>();

	springJumpCamShakeDist = readCfg->at("PlayerStats").at("springJumpCamShakeDist").as<float>();
	springJumpCamShakeTime = readCfg->at("PlayerStats").at("springJumpCamShakeTime").as<float>();

	// Dashing
	maxDashCount = readCfg->at("PlayerStats").at("maxDashCount").as<int>();

	dashSpeed = readCfg->at("PlayerStats").at("dashSpeed").as<float>();
	minDashSpeed = readCfg->at("PlayerStats").at("minDashSpeed").as<float>();
	dashDcclTime = readCfg->at("PlayerStats").at("dashDcclTime").as<float>();
	maxDashTime = readCfg->at("PlayerStats").at("maxDashTime").as<float>();
	dashSlowTime = readCfg->at("PlayerStats").at("dashSlowTime").as<float>();
	dashSlowTimeScale = readCfg->at("PlayerStats").at("dashSlowTimeScale").as<float>();

	dashCamShakeDist = readCfg->at("PlayerStats").at("dashCamShakeDist").as<float>();
	dashCamShakeTime = readCfg->at("PlayerStats").at("dashCamShakeTime").as<float>();

	// Climbing
	climbMaxStamina = readCfg->at("PlayerStats").at("climbMaxStamina").as<float>();
	climbUpCost = readCfg->at("PlayerStats").at("climbUpCost").as<float>();
	climbStillCost = readCfg->at("PlayerStats").at("climbStillCost").as<float>();
	climbJumpCost = readCfg->at("PlayerStats").at("climbJumpCost").as<float>();

	maxClimbUpSpeed = readCfg->at("PlayerStats").at("maxClimbUpSpeed").as<float>();
	maxClimbSlipSpeed = readCfg->at("PlayerStats").at("maxClimbSlipSpeed").as<float>();
	maxClimbDownSpeed = readCfg->at("PlayerStats").at("maxClimbDownSpeed").as<float>();
	minClimbSpeed = readCfg->at("PlayerStats").at("minClimbSpeed").as<float>();
	climbAcclTime = readCfg->at("PlayerStats").at("climbAcclTime").as<float>();
	climbSlipAcclTime = readCfg->at("PlayerStats").at("climbSlipAcclTime").as<float>();
	climbDcclTime = readCfg->at("PlayerStats").at("climbDcclTime").as<float>();

	climbVaultSpeed = readCfg->at("PlayerStats").at("climbVaultSpeed").as<float>();
	climbVaultGravity = readCfg->at("PlayerStats").at("climbVaultGravity").as<float>();
	climbVaultHSpeed = readCfg->at("PlayerStats").at("climbVaultHSpeed").as<float>();
	maxClimbVaultTime = readCfg->at("PlayerStats").at("maxClimbVaultTime").as<float>();

	// Death
	deathCamShakeDist = readCfg->at("PlayerStats").at("deathCamShakeDist").as<float>();
	deathCamShakeTime = readCfg->at("PlayerStats").at("deathCamShakeTime").as<float>();

	// Scene
	exitSpeed = readCfg->at("SceneExit").at("exitSpeed").as<float>();
	outOfBoundsRange = readCfg->at("DeathPit").at("outOfBoundsRange").as<float>();

}
#pragma endregion

#pragma region Event Responders
void MAD::PlayerLogic::InitEventResponders()
{
	playEventHandler.Create([this](const GEvent& _event)
		{
			PlayEvent event;
			PLAY_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case PlayEvent::HIT_SPRING:
			{
				OnHitSpring(data);
				break;
			}
			case PlayEvent::HIT_STRAWBERRY:
			{
				OnHitStrawberry(data);
				break;
			}
			case PlayEvent::HIT_CRYSTAL:
			{
				OnHitCrystal(data);
				break;
			}
			case PlayEvent::HIT_SPIKES:
			{
				OnHitSpikes(data);
				break;
			}
			case PlayEvent::KILL_PLAYER:
			{
				break;
			}
			case PlayEvent::HAPTICS_ACTIVATED:
			{
				OnHapticsActivated(data);
				break;
			}
			case PlayEvent::RELOAD_INI:
			{
				LoadINIStats();
			}
			}
		});
	playEventPusher.Register(playEventHandler);

	levelEventHandler.Create([this](const GEvent& _event)
		{
			LEVEL_EVENT event;
			LEVEL_EVENT_DATA data;

			if (-_event.Read(event, data))
				return;

			switch (event)
			{
			case ENTER_SCENE_BEGIN:
			{
				OnEnterSceneBegin(data);
				break;
			}
			case ENTER_SCENE_DONE:
			{
				OnEnterSceneDone(data);
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
#pragma endregion

#pragma region Init Systems
void MAD::PlayerLogic::InitPlayerControlSystem()
{
	playerControllerSystem = flecsWorld->system<
		Player,
		ControllerID,
		Transform,
		Acceleration,
		Velocity,
		ColliderContainer,
		Moveable,
		FlipInfo>()
		.each([this](
			entity _player,
			Player&,
			ControllerID& _controller,
			Transform& _transform,
			Acceleration& _accel,
			Velocity& _velocity,
			ColliderContainer& _colliders,
			Moveable,
			FlipInfo& _flipInfo)
			{
				if (killPlayer)
					Die();
				if (isExitingScene)
					return;

				float inputX = 0, inputY = 0, inputJump = 0, inputDash = 0, inputClimb = 0;
				float xAxis = 0, yAxis = 0, jumpValue = 0, dashValue = 0, climbValue = 0;

				bool isWindowFocused;
				window.IsFocus(isWindowFocused);
				// Use the controller/keyboard to move the player around the screen
				if (_controller.index == 0 && isWindowFocused)
				{
					bool isControllerConnected;
					gamePadInput.IsConnected(_controller.index, isControllerConnected);

					// Movement controls.
					if (isControllerConnected)
					{
						gamePadInput.GetState(_controller.index, G_LX_AXIS, inputX); xAxis += inputX;
						gamePadInput.GetState(_controller.index, G_LY_AXIS, inputY); yAxis += inputY;

						gamePadInput.GetState(_controller.index, G_SOUTH_BTN, inputJump); jumpValue += inputJump;
						gamePadInput.GetState(_controller.index, G_WEST_BTN, inputDash); dashValue += inputDash;
						gamePadInput.GetState(_controller.index, G_LEFT_TRIGGER_AXIS, inputClimb); climbValue += inputClimb;
					}

					keyboardMouseInput.GetState(G_KEY_LEFT, inputX); xAxis -= inputX;
					keyboardMouseInput.GetState(G_KEY_RIGHT, inputX); xAxis += inputX;
					keyboardMouseInput.GetState(G_KEY_UP, inputY); yAxis += inputY;
					keyboardMouseInput.GetState(G_KEY_DOWN, inputY); yAxis -= inputY;

					keyboardMouseInput.GetState(G_KEY_C, inputJump); jumpValue += inputJump;
					keyboardMouseInput.GetState(G_KEY_X, dashValue); dashValue += inputDash;
					keyboardMouseInput.GetState(G_KEY_Z, climbValue); climbValue += inputClimb;
				}

				HandleOutOfBounds(_player, _transform);
				HandleTriggers(_colliders, _accel, _velocity);

				HandleDashInput(dashValue, xAxis, yAxis, _player, _accel, _velocity);
				HandleClimbInput(climbValue, jumpValue, xAxis, yAxis, _player, _accel, _velocity);
				HandleMovementInput(xAxis, yAxis, _player, _accel, _velocity, _transform, _flipInfo);
				HandleJumpInput(jumpValue, xAxis, _player, _accel, _velocity);

				if (xAxis != 0)
					lastXAxis = xAxis;
				if (yAxis != 0)
					lastYAxis = yAxis;

				isJumpPressed = jumpValue != 0;
				isDashPressed = dashValue != 0;
			});
}
#pragma endregion

#pragma region Event Pushers
void MAD::PlayerLogic::PushPlayEvent(PlayEvent _event, PLAY_EVENT_DATA _data)
{
	GEvent playEvent;
	playEvent.Write(_event, _data);
	playEventPusher.Push(playEvent);
}

void MAD::PlayerLogic::PushCameraEvent(CameraEvent _event, CameraEventData _data)
{
	GEvent cameraEvent;
	cameraEvent.Write(_event, _data);
	cameraEventPusher.Push(cameraEvent);
}

void MAD::PlayerLogic::PushAnimationEvent(ANIM_EVENT _event, float _animSpeed)
{
	GW::GEvent animEvent;
	ANIM_EVENT_DATA data;
	data.animSpeed = _animSpeed;
	animEvent.Write(_event, data);
	animEventPusher.Push(animEvent);
}

void MAD::PlayerLogic::PushTouchEvent(TouchEvent _event, TouchEventData _data)
{
	GW::GEvent touchEvent;
	touchEvent.Write(_event, _data);
	touchEventPusher.Push(touchEvent);
}

#pragma endregion

#pragma region Play Events
void MAD::PlayerLogic::OnHapticsActivated(PLAY_EVENT_DATA _data)
{
	if (playerQuery.count() <= 0)
		return;
	HapticType hapticType = (HapticType)_data.value;
	const Haptics* haptics = playerQuery.first().get<Haptics>();

	if (haptics->info.find(hapticType) != haptics->info.end())
	{
		int controllerId = 0;
		float pan = haptics->info.at(hapticType).pan;
		float duration = haptics->info.at(hapticType).duration;
		float strength = haptics->info.at(hapticType).strength;

		bool isVibrating;
		gamePadInput.IsVibrating(controllerId, isVibrating);
		if (!isVibrating)
		{
			gamePadInput.StartVibration(controllerId, pan, duration, strength);
		}
	}
}

void MAD::PlayerLogic::OnHitSpring(PLAY_EVENT_DATA _data)
{
	hitSpring = true;
	ReplenishStats();
	ActivateHaptics(HapticType::SPRING_BOUNCE);
}

void MAD::PlayerLogic::OnHitStrawberry(PLAY_EVENT_DATA _data)
{
	hitStrawberry = true;
}

void MAD::PlayerLogic::OnHitCrystal(PLAY_EVENT_DATA _data)
{
	if (CanReplenishStats())
	{
		ReplenishStats();
		PushPlayEvent(PlayEvent::COLLECT_CRYSTAL, _data);
	}
}

void MAD::PlayerLogic::OnHitSpikes(PLAY_EVENT_DATA _data)
{
	OnKillPlayer(_data);
}

void MAD::PlayerLogic::OnKillPlayer(PLAY_EVENT_DATA _data)
{
	killPlayer = true;
}
#pragma endregion

#pragma region Level Events
void MAD::PlayerLogic::OnEnterSceneBegin(LEVEL_EVENT_DATA _data)
{
	if (playerQuery.count() == 0 || !playerQuery.first().is_alive())
		return;

	flecsWorld->defer_begin();

	// Find the velocity to launch the player with
	GVECTORF sceneExitPos = _data.sceneExit.get<Transform>()->value.row4;
	GVECTORF closestTilePos;
	GVECTORF launchVel = GZeroVectorF;
	if (saveLoader->GetClosestTilePosOfScene(sceneExitPos, _data.sceneIndex, closestTilePos));
	{
		GVector::SubtractVectorF(closestTilePos, sceneExitPos, launchVel);
		GVector::NormalizeF(launchVel, launchVel);
		GVector::ScaleF(launchVel, exitSpeed, launchVel);
	}

	GVECTORF velocity = playerQuery.first().get<Velocity>()->value;
	if (launchVel.x != 0)
		velocity.x = launchVel.x;
	if (launchVel.y != 0)
		velocity.y = launchVel.y;
	playerQuery.first().set<Velocity>({ velocity });
	playerQuery.first().set<Acceleration>({ GZeroVectorF });

	flecsWorld->defer_end();

	StopDash();
	ReplenishStats();

	isExitingScene = true;
}

void MAD::PlayerLogic::OnEnterSceneDone(LEVEL_EVENT_DATA _data)
{
	isExitingScene = false;
}
#pragma endregion


#pragma region Moving
void MAD::PlayerLogic::HandleMovementInput(
	float _xAxis,
	float _yAxis,
	flecs::entity _entity,
	Acceleration& _acceleration,
	Velocity& _velocity,
	Transform& _transform,
	FlipInfo& _flipInfo)
{
	switch (controlState)
	{
	case MAD::PlayerLogic::NORMAL:
	{
		if (!isWallJumping)
		{
			// Run Accel
			if (_xAxis != 0)
			{
				_acceleration.value.x = (maxRunSpeed / runAcclTime) * _xAxis;

				if (isGrounded && _velocity.value.x == 0)
					_velocity.value.x = maxRunSpeed * _xAxis;
			}
			else if (_velocity.value.x != 0) // Run Dccel
			{
				if (abs(_velocity.value.x) < minRunSpeed)
				{
					_acceleration.value.x = 0;
					_velocity.value.x = 0;
				}
				else
					_acceleration.value.x = (maxRunSpeed / runDcclTime) * SIGN(_velocity.value.x) * -1;
			}

			// Limit run speed
			if (abs(_velocity.value.x) > maxRunSpeed)
			{
				_acceleration.value.x = 0;
				_velocity.value.x = maxRunSpeed * SIGN(_velocity.value.x);
			}

			// Run animation
			if (isGrounded)
			{
				if (_velocity.value.x != 0 && _xAxis != 0)
					PlayAnimation(ANIM_EVENT::PLAYER_IS_RUNNING, abs(_velocity.value.x) / maxRunSpeed);
				else
					PlayAnimation(ANIM_EVENT::PLAYER_IS_IDLE, 1);
			}

			// Flipping
			FlipEntity(_entity.delta_time(), -_xAxis, _transform, _flipInfo);
		}
		break;
	}
	case MAD::PlayerLogic::DASHING:
	{
		if (_xAxis != 0 && isDashDccling)
		{
			_acceleration.value.x += (maxRunSpeed / runAcclTime) * _xAxis;
		}
		break;
	}
	}
}

void MAD::PlayerLogic::FlipEntity(float _deltaTime, float _dirMoving, Transform& _transform, FlipInfo& _flipInfo)
{
	// If the dir we want to move (_dirMoving) is different from isFacingRight, start a flip
	if ((_flipInfo.isFacingRight && _dirMoving < 0)
		|| (!_flipInfo.isFacingRight && _dirMoving > 0))
	{
		_flipInfo.isFacingRight = _dirMoving > 0;

		if (_flipInfo.flipDir == 0) // If we're not flipping (0) start flipping left or right
			_flipInfo.flipDir = _dirMoving > 0 ? 1 : -1;
	}

	if (_flipInfo.flipDir != 0)
	{
		// Get the amount to rotate this frame w/o a direction
		float rotAmount = 180
			* (1000.0f / _flipInfo.flipTime)
			* _deltaTime;

		if (_flipInfo.flipDir == 1) // If rotating towards the right
		{
			rotAmount = -rotAmount; // - rotAmount because we're going from 180 -> 0
			_flipInfo.degreesFlipped += rotAmount;

			if (_flipInfo.degreesFlipped < 0) // if we've overshot 0 degrees, correct the difference
			{
				rotAmount -= _flipInfo.degreesFlipped;
				_flipInfo.degreesFlipped = 0;

				if (_flipInfo.isFacingRight)	// if we're currently wanting to go right after flip is done, stop flipping
					_flipInfo.flipDir = 0;
				else							// else start flipping back left
					_flipInfo.flipDir = -1;
			}
		}
		else // Else rotating towards the left
		{
			_flipInfo.degreesFlipped += rotAmount; // + rotAmount because we're going 0 -> 180

			if (_flipInfo.degreesFlipped > 180) // if we've overshot 180 degrees, correct the difference
			{
				rotAmount -= _flipInfo.degreesFlipped - 180;
				_flipInfo.degreesFlipped = 180;

				if (!_flipInfo.isFacingRight)	// if we're currently wanting to go left after flip is done, stop flipping
					_flipInfo.flipDir = 0;
				else							// else start flipping back right
					_flipInfo.flipDir = 1;
			}
		}

		GW::MATH::GMatrix::RotateYLocalF(_transform.value, G_DEGREE_TO_RADIAN(rotAmount), _transform.value);
	}
}
#pragma endregion

#pragma region Jumping
void PlayerLogic::HandleJumpInput(
	float _jumpInput,
	float _xAxis,
	flecs::entity _entity,
	Acceleration& _acceleration,
	Velocity& _velocity)
{
	if (hitSpring)
		SpringJump(_acceleration, _velocity);

	switch (controlState)
	{
	case ControlState::NORMAL:
	{
		// Gravity
		if (isGrounded)
		{
			_acceleration.value.y = 0;
		}
		else
		{
			if (!isJumping)
			{
				if (!isWallSliding)
					_acceleration.value.y = -gravityScale;
				else
					_acceleration.value.y = -wallSlideGravityScale;
			}
			else if (isSpringJumping)
				_acceleration.value.y = -springJumpGravityScale;
			else if ((_jumpInput != 0 && _velocity.value.y > 0))
				_acceleration.value.y = -jumpGravityScale;
			else
				_acceleration.value.y = -stopJumpGravityScale;
		}

		// Jumping
		if (!isJumping)
		{
			if (_jumpInput != 0 && !isJumpPressed)
			{
				if (isGrounded)
					Jump(_velocity);
				else if (CanBufferJump())
					Jump(_velocity);
				else if (CanWallJump())
					WallJump(_acceleration, _velocity, isTouchingLeftWall ? 1 : -1);
				else if (!isJumpQueued)
					StartJumpQueue();
			}
			else if (CanQueueJump())
				Jump(_velocity);
			else if (canQueueWallJump())
				WallJump(_acceleration, _velocity, isTouchingLeftWall ? 1 : -1);
		}
		else if (_velocity.value.y <= 0)
		{
			isJumping = false;
			isSpringJumping = false;

			if (!isGrounded)
				PlayAnimation(ANIM_EVENT::PLAYER_IS_FALLING, 1);			
		}

		if (isWallJumping)
		{
			wallJumpTime += _entity.delta_time();
			if (wallJumpTime > maxWallJumpTime)
				isWallJumping = false;
		}

		// Velocity limit
		if (isWallSliding && _velocity.value.y < -maxWallSlideSpeed)
			_velocity.value.y = -maxWallSlideSpeed;
		if (abs(_velocity.value.y) > maxYSpeed && !isSpringJumping)
			_velocity.value.y = maxYSpeed * SIGN(_velocity.value.y);
		break;
	}
	case ControlState::DASHING:
	{
		if (!isDashJumping)
		{
			// Jumping
			if (_jumpInput != 0 && !isJumpPressed)
			{
				if (isGrounded)
					Jump(_velocity);
				else if (CanBufferJump())
					Jump(_velocity);
				else if (!isJumpQueued)
					StartJumpQueue();
			}
			else if (CanQueueJump())
				Jump(_velocity);
		}
		else if (!isGrounded)
		{
			// Gravity
			if (!isJumping)
			{
				if (!isWallSliding)
					_acceleration.value.y = -gravityScale;
				else
					_acceleration.value.y = -wallSlideGravityScale;
			}
			else if (_jumpInput != 0 && _velocity.value.y > 0)
				_acceleration.value.y = -jumpGravityScale;
			else
				_acceleration.value.y = -stopJumpGravityScale;
		}

		break;
	}
	}

	// Buffer and Queue Timers
	if (!isGrounded)
		timeSinceGrounded += _entity.delta_time();
	if (isJumpQueued)
		jumpQueueTime += _entity.delta_time();

}

void PlayerLogic::Jump(Velocity& _velocity)
{
	isJumping = true;
	hasJumped = true;
	isJumpBuffered = false;
	isJumpQueued = false;

	_velocity.value.y = jumpSpeed;

	playerQuery.first().get<SoundClips>()->PlaySound("Jump");

	ActivateHaptics(HapticType::JUMP);

	if (_velocity.value.x == 0)
		PlayAnimation(ANIM_EVENT::PLAYER_JUMPED, 1);
	else
		PlayAnimation(ANIM_EVENT::PLAYER_RUN_JUMP, 0.8f);
}

void MAD::PlayerLogic::WallJump(Acceleration& _acceleration, Velocity& _velocity, float dir)
{
	isWallJumping = true;
	wallJumpTime = 0;
	_acceleration.value.x = 0;
	_velocity.value.x = dir * wallJumpHSpeed;

	Jump(_velocity);

	PlayAnimation(ANIM_EVENT::PLAYER_WALL_JUMP, 1.5f);
}

void MAD::PlayerLogic::SpringJump(Acceleration& _acceleration, Velocity& _velocity)
{
	if (controlState == ControlState::DASHING)
		StopDash();

	controlState = ControlState::NORMAL;
	hitSpring = false;
	isJumping = true;
	isSpringJumping = true;

	_velocity.value.x = 0;
	_velocity.value.y = springJumpSpeed;

	ShakeCamera(GVECTORF({0, 1}), springJumpCamShakeDist, springJumpCamShakeTime);
}

bool MAD::PlayerLogic::CanWallJump()
{
	return isTouchingLeftWall || isTouchingRightWall;
}

void MAD::PlayerLogic::StartJumpQueue()
{
	isJumpQueued = true;
	jumpQueueTime = 0;
}

bool MAD::PlayerLogic::CanBufferJump()
{
	return timeSinceGrounded < maxJumpBufferTime && !hasJumped;
}

bool MAD::PlayerLogic::CanQueueJump()
{
	return isJumpQueued == true && jumpQueueTime < maxJumpQueueTime && isGrounded;
}

bool MAD::PlayerLogic::canQueueWallJump()
{
	return isJumpQueued == true && jumpQueueTime < maxJumpQueueTime && (isTouchingLeftWall || isTouchingRightWall);
}

void PlayerLogic::HitGround(
	Acceleration& _acceleration,
	Velocity& _velocity)
{
	isGrounded = true;
	timeSinceGrounded = 0;

	if (_velocity.value.y < 0)
	{
		hasJumped = false;
		_velocity.value.y = 0;
		_acceleration.value.y = 0;

		playerQuery.first().get<SoundClips>()->PlaySound("HitGround");

		if (!isGrounded)
			ActivateHaptics(HapticType::LAND_GROUND);

		if (hitStrawberry)
		{
			PushPlayEvent(PlayEvent::COLLECT_STRAWBERRIES, {});
		}

		PlayAnimation(ANIM_EVENT::PLAYER_LANDED, 1);
	}

	ReplenishStats();
}

void MAD::PlayerLogic::LeaveGround()
{
	isGrounded = false;
}
#pragma endregion

#pragma region Dashing
void PlayerLogic::HandleDashInput(
	float _dashInput,
	float _xAxis,
	float _yAxis,
	flecs::entity _entity,
	Acceleration& _acceleration,
	Velocity& _velocity)
{
	switch (controlState)
	{
	case ControlState::NORMAL:
	{
		if (_dashInput != 0 && !isDashPressed && dashCount > 0)
			Dash(_velocity, _acceleration, _xAxis, _yAxis);
		break;
	}
	case MAD::PlayerLogic::DASHING:
	{
		dashTime += _entity.delta_time();

		if (dashTime > dashSlowTime)
			flecsWorld->set_time_scale(1);

		if (dashTime >= maxDashTime)
		{
			isDashDccling = true;
			if (dashDir.y > 0)
				_acceleration.value.y = (dashSpeed / dashDcclTime) * -dashDir.y;
			_acceleration.value.x = (dashSpeed / dashDcclTime) * -dashDir.x;

			if (dashTime >= maxDashTime + dashDcclTime
				|| SIGN(_velocity.value.x) != SIGN(dashDir.x)
				|| SIGN(_velocity.value.y) != SIGN(dashDir.y))
				StopDash();

			if (_dashInput != 0 && !isDashPressed && dashCount > 0)
			{
				StopDash();
				Dash(_velocity, _acceleration, _xAxis, _yAxis);
			}
		}
		break;
	}
	case ControlState::CLIMBING:
	{
		if (_dashInput != 0 && !isDashPressed && dashCount > 0)
		{
			StopClimb();
			Dash(_velocity, _acceleration, _xAxis, _yAxis);
		}
		break;
	}
	}
}

void PlayerLogic::Dash(
	Velocity& _velocity,
	Acceleration& _acceleration,
	float _xAxis,
	float _yAxis)
{
	controlState = ControlState::DASHING;
	dashCount--;
	dashTime = 0;
	flecsWorld->set_time_scale(dashSlowTimeScale);

	if (_xAxis != 0 || _yAxis != 0)
		dashDir = { _xAxis, _yAxis };
	else
		dashDir = { lastXAxis, 0 };
	GVector::NormalizeF(dashDir, dashDir);
	GVector::ScaleF(dashDir, dashSpeed, _velocity.value);

	_acceleration.value = {};

	playerQuery.first().get<SoundClips>()->PlaySound("Dash");
	ActivateHaptics(HapticType::DASH);
	ShakeCamera(dashDir, -dashCamShakeDist, dashCamShakeTime);
	
	if (dashDir.y == 0)
		PlayAnimation(ANIM_EVENT::PLAYER_DASH_LR, 1);
	else if (dashDir.x == 0)
	{
		if (dashDir.y < 0)
			PlayAnimation(ANIM_EVENT::PLAYER_DASH_DOWN, 1);
		else
			PlayAnimation(ANIM_EVENT::PLAYER_DASH_UP, 1);
	}
	else
	{
		if (dashDir.y < 0)
			PlayAnimation(ANIM_EVENT::PLAYER_DASH_DIAGONAL_DOWN, 1);
		else
			PlayAnimation(ANIM_EVENT::PLAYER_DASH_DIAGONAL_UP, 1);
	}
}

void MAD::PlayerLogic::StopDash()
{
	controlState = ControlState::NORMAL;
	isDashDccling = false;
	isDashJumping = false;
	flecsWorld->set_time_scale(1);
}
#pragma endregion

#pragma	region Climbing
void PlayerLogic::HandleClimbInput(
	float _climbInput,
	float _jumpInput,
	float _xAxis,
	float _yAxis,
	flecs::entity _entity,
	Acceleration& _acceleration,
	Velocity& _velocity)
{
	switch (controlState)
	{
	case ControlState::NORMAL:
	{
		if (_climbInput != 0 && CanStartClimb(_xAxis))
			StartClimb(_acceleration, _velocity, _xAxis);
		else if (_xAxis != 0 && CanStartClimb(_xAxis) && _velocity.value.y < 0)
			StartWallSlide(_velocity);
		else
			StopWallSlide();
		break;
	}
	case ControlState::DASHING:
	{
		if (_climbInput != 0 && CanStartClimb(_xAxis))
			StartClimb(_acceleration, _velocity, _xAxis);
		break;
	}
	case ControlState::CLIMBING:
	{
		if (isClimbVaulting)
		{
			if (_climbInput != 0 && CanStartClimb(_xAxis))
			{
				StopClimb();
				StartClimb(_acceleration, _velocity, _xAxis);
			}

			climbVaultTime += _entity.delta_time();
			_velocity.value.x = climbVaultHSpeed * climbVaultDir;
			if (climbVaultTime > maxClimbVaultTime)
				StopClimb();
		}
		else if (_climbInput == 0)
			StopClimb();
		else if (climbStamina > 0)
		{
			if (_jumpInput != 0 && !isJumpPressed) // Wall jump
			{
				StopClimb();
				if (_xAxis != 0)
					WallJump(_acceleration, _velocity, isTouchingLeftWall ? 1 : -1);
				else
				{
					climbStamina -= climbJumpCost;
					Jump(_velocity);
				}
			}
			else if (_yAxis > 0) // Climb up
			{
				climbStamina -= climbUpCost * _entity.delta_time();

				if (_velocity.value.y > maxClimbUpSpeed)
				{
					_velocity.value.y = maxClimbUpSpeed;
					_acceleration.value.y = 0;
				}
				else
					_acceleration.value.y = maxClimbUpSpeed / climbAcclTime;

				PlayAnimation(ANIM_EVENT::PLAYER_IS_CLIMBING, 2.0f);
			}
			else if (_yAxis < 0) // Climb down
			{
				if (_velocity.value.y < -maxClimbDownSpeed)
				{
					_velocity.value.y = -maxClimbDownSpeed;
					_acceleration.value.y = 0;
				}
				else
					_acceleration.value.y = -maxClimbDownSpeed / climbAcclTime;

				PlayAnimation(ANIM_EVENT::PLAYER_IS_HANGING, 1);
			}
			else // Dccel
			{
				climbStamina -= climbStillCost * _entity.delta_time();

				if (abs(_velocity.value.y) < minClimbSpeed)
				{
					_velocity.value.y = 0;
					_acceleration.value.y = 0;
				}
				else
				{
					_acceleration.value.y = (maxClimbUpSpeed / climbDcclTime) * -SIGN(_velocity.value.y);
				}

				PlayAnimation(ANIM_EVENT::PLAYER_IS_HANGING, 1);
			}
		}
		else // No stamina
		{
			if (_jumpInput != 0 && !isJumpPressed) // Wall jump
			{
				StopClimb();
				WallJump(_acceleration, _velocity, isTouchingLeftWall ? 1 : -1);
			}
			else // Slip down wall
			{
				if (_velocity.value.y < -maxClimbSlipSpeed)
				{
					_velocity.value.y = -maxClimbSlipSpeed;
					_acceleration.value.y = 0;
				}
				else
					_acceleration.value.y = -maxClimbSlipSpeed / climbSlipAcclTime;
			}

			PlayAnimation(ANIM_EVENT::PLAYER_IS_HANGING, 1);
		}
		break;
	}
	}

}

bool PlayerLogic::CanStartClimb(float _xAxis)
{
	float climbDir = _xAxis != 0 ? _xAxis : lastXAxis;

	return !isJumping && ((climbDir > 0 && isTouchingRightWall) || (climbDir < 0 && isTouchingLeftWall));
}

void MAD::PlayerLogic::StartClimb(Acceleration& _acceleration, Velocity& _velocity, float _xAxis)
{
	if (controlState == ControlState::DASHING)
		StopDash();
	controlState = ControlState::CLIMBING;

	float climbDir = _xAxis != 0 ? _xAxis : lastXAxis;
	isClimbingRightWall = climbDir > 0;

	_acceleration.value = {};
	_velocity.value = {};

	if (isClimbingRightWall)
	{
		for (int i = 0; i < rightObjectsTouching.size(); ++i)
			PushTouchEvent(TouchEvent::ENTER_CLIMB, { rightObjectsTouching[i] });
	}
	else
	{
		for (int i = 0; i < leftObjectsTouching.size(); ++i)
			PushTouchEvent(TouchEvent::ENTER_CLIMB, { leftObjectsTouching[i] });
	}

	PlayAnimation(ANIM_EVENT::PLAYER_IS_HANGING, 1);
}

void MAD::PlayerLogic::StopClimb()
{
	controlState = ControlState::NORMAL;
	isClimbVaulting = false;

	if (isClimbingRightWall)
	{
		for (int i = 0; i < rightObjectsTouching.size(); ++i)
			PushTouchEvent(TouchEvent::EXIT_CLIMB, { rightObjectsTouching[i] });
	}
	else
	{
		for (int i = 0; i < leftObjectsTouching.size(); ++i)
			PushTouchEvent(TouchEvent::EXIT_CLIMB, { leftObjectsTouching[i] });
	}
}

void MAD::PlayerLogic::FallOffClimb(Acceleration& _acceleration, Velocity& _velocity, float _dir)
{
	if (_velocity.value.y > 0)
		ClimbVault(_acceleration, _velocity, _dir);
	else
		StopClimb();
}

void MAD::PlayerLogic::ClimbVault(Acceleration& _acceleration, Velocity& _velocity, float _dir)
{
	isClimbVaulting = true;
	climbVaultTime = 0;
	climbVaultDir = _dir;

	_velocity.value.x = climbVaultHSpeed * _dir;
	_velocity.value.y = climbVaultSpeed;
	_acceleration.value.y = -climbVaultGravity;
}

void MAD::PlayerLogic::StartWallSlide(Velocity& _velocity)
{
	if (!isWallSliding)
	{
		isWallSliding = true;

		if (_velocity.value.y < -minWallSlideSpeed / wallSlideSpeedMult)
			_velocity.value.y *= wallSlideSpeedMult;
	}
}

void MAD::PlayerLogic::StopWallSlide()
{
	isWallSliding = false;
}

void MAD::PlayerLogic::HitWall(WallSide _wallSide)
{
	if (_wallSide == WallSide::LEFT)
		isTouchingLeftWall = true;
	else
		isTouchingRightWall = true;
}

void MAD::PlayerLogic::LeaveWall(WallSide _wallSide, Acceleration& _acceleration, Velocity& _velocity)
{
	if (_wallSide == WallSide::LEFT)
		isTouchingLeftWall = false;
	else
		isTouchingRightWall = false;

	if (controlState == ControlState::CLIMBING && !isTouchingLeftWall && !isTouchingRightWall)
	{
		FallOffClimb(_acceleration, _velocity, _wallSide == WallSide::LEFT ? -1 : 1);
	}
}
#pragma endregion


#pragma region Triggers

void PlayerLogic::HandleTriggers(ColliderContainer& _colliderContainer,
	Acceleration& _acceleration,
	Velocity& _velocity)
{
	// ground trigger
	UpdateGroundObjectsTouching(_colliderContainer.triggerColliders[groundTriggerId]->contacts);
	if (_colliderContainer.triggerColliders[groundTriggerId]->contacts.size() > 0)
	{
		HitGround(_acceleration, _velocity);
	}
	else if (isGrounded)
	{
		LeaveGround();
	}

	// right trigger
	UpdateRightObjectsTouching(_colliderContainer.triggerColliders[rightTriggerId]->contacts);
	if (_colliderContainer.triggerColliders[rightTriggerId]->contacts.size() > 0)
	{
		if (!isTouchingRightWall)
			HitWall(WallSide::RIGHT);
	}
	else if (isTouchingRightWall)
	{
		LeaveWall(WallSide::RIGHT, _acceleration, _velocity);
	}

	// left trigger
	UpdateLeftObjectsTouching(_colliderContainer.triggerColliders[leftTriggerId]->contacts);
	if (_colliderContainer.triggerColliders[leftTriggerId]->contacts.size() > 0)
	{
		if (!isTouchingLeftWall)
			HitWall(WallSide::LEFT);
	}
	else if (isTouchingLeftWall)
	{
		LeaveWall(WallSide::LEFT, _acceleration, _velocity);
	}
}

void MAD::PlayerLogic::UpdateGroundObjectsTouching(std::vector<Collider*>& _colliders)
{
	std::vector<flecs::id> enteredCollisions;
	std::vector<flecs::id> exitedCollisions;
	GetEnteredCollisions(groundObjectsTouching, _colliders, enteredCollisions);
	GetExitedCollisions(groundObjectsTouching, _colliders, exitedCollisions);

	for (int i = 0; i < enteredCollisions.size(); ++i)
		PushTouchEvent(TouchEvent::ENTER_STAND, { enteredCollisions[i] });

	for (int i = 0; i < exitedCollisions.size(); ++i)
		PushTouchEvent(TouchEvent::EXIT_STAND, { exitedCollisions[i] });

	UpdateObjectsTouching(groundObjectsTouching, _colliders);
}

void MAD::PlayerLogic::UpdateLeftObjectsTouching(std::vector<Collider*>& _colliders)
{
	if (controlState == ControlState::CLIMBING && !isClimbingRightWall)
	{
		std::vector<flecs::id> enteredCollisions;
		std::vector<flecs::id> exitedCollisions;
		GetEnteredCollisions(leftObjectsTouching, _colliders, enteredCollisions);
		GetExitedCollisions(leftObjectsTouching, _colliders, exitedCollisions);

		for (int i = 0; i < enteredCollisions.size(); ++i)
			PushTouchEvent(TouchEvent::ENTER_CLIMB, { enteredCollisions[i] });

		for (int i = 0; i < exitedCollisions.size(); ++i)
			PushTouchEvent(TouchEvent::EXIT_CLIMB, { exitedCollisions[i] });
	}

	UpdateObjectsTouching(leftObjectsTouching, _colliders);
}

void MAD::PlayerLogic::UpdateRightObjectsTouching(std::vector<Collider*>& _colliders)
{
	if (controlState == ControlState::CLIMBING && isClimbingRightWall)
	{
		std::vector<flecs::id> enteredCollisions;
		std::vector<flecs::id> exitedCollisions;
		GetEnteredCollisions(rightObjectsTouching, _colliders, enteredCollisions);
		GetExitedCollisions(rightObjectsTouching, _colliders, exitedCollisions);

		for (int i = 0; i < enteredCollisions.size(); ++i)
			PushTouchEvent(TouchEvent::ENTER_CLIMB, { enteredCollisions[i] });

		for (int i = 0; i < exitedCollisions.size(); ++i)
			PushTouchEvent(TouchEvent::EXIT_CLIMB, { exitedCollisions[i] });
	}

	UpdateObjectsTouching(rightObjectsTouching, _colliders);
}

void MAD::PlayerLogic::UpdateObjectsTouching(std::vector<flecs::id>& _touchingVector, std::vector<Collider*>& _colliders)
{
	if (_colliders.size() == 0)
	{
		if (_touchingVector.size() > 0)
			_touchingVector.clear();
		return;
	}

	if (_touchingVector.size() != _colliders.size())
		_touchingVector.resize(_colliders.size());

	for (int i = 0; i < _colliders.size(); ++i)
	{
		_touchingVector[i] = _colliders[i]->ownerId;
	}
}

void MAD::PlayerLogic::GetEnteredCollisions(std::vector<flecs::id>& _touchingVector, std::vector<Collider*>& _colliders, std::vector<flecs::id>& _outIds)
{
	for (int i = 0; i < _colliders.size(); i++)
	{
		bool foundId = false;
		for (int j = 0; j < _touchingVector.size(); j++)
		{
			if (_colliders[i]->ownerId == _touchingVector[j])
			{
				foundId = true;
				break;
			}
		}

		if (!foundId)
			_outIds.push_back(_colliders[i]->ownerId);
	}
}

void MAD::PlayerLogic::GetExitedCollisions(std::vector<flecs::id>& _touchingVector, std::vector<Collider*>& _colliders, std::vector<flecs::id>& _outIds)
{
	for (int i = 0; i < _touchingVector.size(); i++)
	{
		bool foundId = false;
		for (int j = 0; j < _colliders.size(); j++)
		{
			if (_touchingVector[i] == _colliders[j]->ownerId)
			{
				foundId = true;
				break;
			}
		}

		if (!foundId)
			_outIds.push_back(_touchingVector[i]);
	}
}
#pragma endregion

#pragma region Death
void MAD::PlayerLogic::HandleOutOfBounds(flecs::entity _entity, Transform& _transform)
{
	if (!saveLoader->GetScene(saveLoader->GetSaveSlot().sceneIndex)->IsPointInRange(outOfBoundsRange, _transform.value.row4))
	{
		int sceneIndex = saveLoader->GetSceneAtPoint(_transform.value.row4);
		if (sceneIndex == -1)
		{
			Die();
		}
	}
}

void MAD::PlayerLogic::Die()
{
	killPlayer = false;
	GVECTORF camShakeDir = playerQuery.first().get<Velocity>()->value;
	playerQuery.first().get<SoundClips>()->PlaySound("Die");
	playerQuery.first().remove<Collidable>();
	playerQuery.first().remove<RenderModel>();
	playerQuery.first().remove<Moveable>();
	playerQuery.first().set<Acceleration>({});
	playerQuery.first().set<Velocity>({});
	auto colliderContainer = *playerQuery.first().get<ColliderContainer>();
	colliderContainer.DropAllContacts();
	playerQuery.first().set<ColliderContainer>(colliderContainer);
	saveLoader->PlayerDied();
	hitStrawberry = false;
	hitSpring = false;
	StopDash();
	ReplenishStats();
	PushPlayEvent(PlayEvent::PLAYER_DESTROYED, {});
	ActivateHaptics(HapticType::PLAYER_DEATH);
	if (camShakeDir.x != 0 || camShakeDir.y != 0)
		GVector::NormalizeF(camShakeDir, camShakeDir);
	else
		camShakeDir = GVECTORF({ 1 });
	ShakeCamera(camShakeDir, deathCamShakeDist, deathCamShakeTime);
}
#pragma endregion

#pragma region Stats
void MAD::PlayerLogic::ReplenishStats()
{
	dashCount = maxDashCount;
	climbStamina = climbMaxStamina;
}

bool MAD::PlayerLogic::CanReplenishStats()
{
	return dashCount < maxDashCount 
		|| climbStamina < climbMaxStamina;
}
#pragma endregion

#pragma region Haptics
void MAD::PlayerLogic::ActivateHaptics(HapticType _type)
{
	PLAY_EVENT_DATA hapticData{};
	hapticData.value = _type;
	PushPlayEvent(PlayEvent::HAPTICS_ACTIVATED, hapticData);
}
#pragma endregion

#pragma region Animation
void MAD::PlayerLogic::PlayAnimation(ANIM_EVENT animEvent, float animSpeed)
{
	if (animEvent != curAnimEvent || animSpeed != curAnimSpeed)
	{
		curAnimEvent = animEvent;
		curAnimSpeed = animSpeed;
		PushAnimationEvent(animEvent, animSpeed);
	}
}
#pragma endregion

#pragma region Effects
void MAD::PlayerLogic::ShakeCamera(GVECTORF _dir, float _distance, float _time)
{
	PushCameraEvent(CameraEvent::SHAKE, { _dir, _distance, _time });
}
#pragma endregion


#pragma region Shutdown / Activate
bool PlayerLogic::Shutdown()
{
	flecsWorldAsync.merge();

	playerControllerSystem.destruct();
	playerQuery.destruct();
	flecsWorld.reset();
	gameConfig.reset();

	return true;
}

void MAD::PlayerLogic::GameplayStop()
{
	groundObjectsTouching.clear();
	rightObjectsTouching.clear();
	leftObjectsTouching.clear();
}

bool PlayerLogic::Activate(bool _runSystem)
{
	if (playerControllerSystem.is_alive())
	{
		(_runSystem) ? playerControllerSystem.enable() : playerControllerSystem.disable();
		return true;
	}
	return false;
}
#pragma endregion