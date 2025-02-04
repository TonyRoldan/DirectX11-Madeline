#include "AnimationLogic.h"

using namespace MAD;

void MAD::AnimationLogic::Init(DirectX11Renderer* _renderer, 
								std::shared_ptr<flecs::world> _flecsWorld, 
								GW::CORE::GEventGenerator _animEventPusher,
								GW::CORE::GEventGenerator _gameStateEventPusher)
{
	renderer = _renderer;
	animEventPusher = _animEventPusher;
	flecsWorld = _flecsWorld;
	gameStateEventPusher = _gameStateEventPusher;

	currEvent = PLAYER_IS_IDLE;
	renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::IDLE;

	CreateEvents();
}

void MAD::AnimationLogic::CreateEvents()
{
	animEventResponder.Create([this](const GW::GEvent& _event)
		{
			ANIM_EVENT eventTag;
			ANIM_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{	
				renderer->modelAnimSpeed = data.animSpeed;
				if (currEvent == eventTag)
					return;
				renderer->animationState.isBlending = false; // stops any current blending 
					
				
				switch (eventTag)
				{
					case ANIM_EVENT::PLAYER_IS_RUNNING:
					{
						if (currEvent == ANIM_EVENT::PLAYER_IS_IDLE)
						{
							Transition(PLAYER_ANIMATIONS::RUN, 0.1f);
						}
						else if (currEvent == ANIM_EVENT::PLAYER_LANDED)
						{
							Transition(PLAYER_ANIMATIONS::RUN, 0.1f);
						}
						else
						{
							renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::RUN;
						}	
						break;
					}
					case ANIM_EVENT::PLAYER_IS_CLIMBING:
					{
						if (currEvent == ANIM_EVENT::PLAYER_IS_HANGING)
						{
							Transition(PLAYER_ANIMATIONS::CLIMB_UP, 0.1f);
						}
						else
						{
							renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::CLIMB_UP;
						}
						break;
					}
					case ANIM_EVENT::PLAYER_IS_HANGING:
					{
						if (currEvent == ANIM_EVENT::PLAYER_IS_CLIMBING)
						{
							Transition(PLAYER_ANIMATIONS::IDLE_HANG, 0.15f);
						}
						else
						{
							renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::IDLE_HANG;
						}
						break;
					}
					case ANIM_EVENT::PLAYER_IS_FALLING:
					{
						if (currEvent == ANIM_EVENT::PLAYER_RUN_JUMP)
						{
							Transition(PLAYER_ANIMATIONS::FALLING, 0.5f);
						}
						else
						{
							renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::FALLING;
						}	
						break;
					}
					case ANIM_EVENT::PLAYER_JUMPED:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::STANDING_JUMP;
						break;
					}
					case ANIM_EVENT::PLAYER_RUN_JUMP:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::RUNNING_JUMP;
						break;
					}
					case ANIM_EVENT::PLAYER_DIED:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DEATH;
						break;
					}
					case ANIM_EVENT::PLAYER_WALL_JUMP:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::WALL_JUMP;
						break;
					}
					case ANIM_EVENT::PLAYER_IS_IDLE:
					{						
						if (currEvent == ANIM_EVENT::PLAYER_IS_RUNNING)
						{
							Transition(PLAYER_ANIMATIONS::IDLE, 0.15f);
						}
						else if (currEvent == ANIM_EVENT::PLAYER_LANDED)
						{
							Transition(PLAYER_ANIMATIONS::IDLE, 0.15f);
						}
						else
						{
							renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::IDLE;
						}
						break;
					}
					case ANIM_EVENT::PLAYER_LANDED:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::LANDING;
						break;
					}
					case ANIM_EVENT::PLAYER_DASH_LR:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DASH_LR;
						break;
					}
					case ANIM_EVENT::PLAYER_DASH_UP:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DASH_UP;
						break;
					}
					case ANIM_EVENT::PLAYER_DASH_DOWN:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DASH_DOWN;
						break;
					}
					case ANIM_EVENT::PLAYER_DASH_DIAGONAL_UP:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DASH_DIAGONAL_UP;
						break;
					}
					case ANIM_EVENT::PLAYER_DASH_DIAGONAL_DOWN:
					{
						renderer->animationState.currentAnimNDX = PLAYER_ANIMATIONS::DASH_DIAGONAL_DOWN;
						break;
					}
					default:
					{
						break;
					}				
				}
				currEvent = eventTag;
			}
		});
	animEventPusher.Register(animEventResponder);

	gameStateEventResponder.Create([this](const GW::GEvent& _event)
		{
			GAME_STATE eventTag;
			GAME_STATE_EVENT_DATA data;

			if (+_event.Read(eventTag, data))
			{
				if (eventTag == GAME_STATE::PAUSE_GAME)
				{
					renderer->modelAnimPause = true;
				}
				else if(eventTag == GAME_STATE::PLAY_GAME)
				{
					renderer->modelAnimPause = false;
				}
			}
		});
	gameStateEventPusher.Register(gameStateEventResponder);
}

void MAD::AnimationLogic::Transition(PLAYER_ANIMATIONS nextAnim, float transitionLength)
{
	renderer->animationState.isBlending = true;
	renderer->animationState.nextAnimNdx = nextAnim;
	renderer->transitionLength = transitionLength;

}

#pragma region Activate / Shutdown
bool MAD::AnimationLogic::Activate(bool runSystem)
{
	return true;
}

bool MAD::AnimationLogic::Shutdown()
{
	flecsWorld.reset();

	return true;
}
#pragma endregion



