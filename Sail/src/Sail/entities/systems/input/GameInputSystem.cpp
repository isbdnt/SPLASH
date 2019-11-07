#include "pch.h"
#include "GameInputSystem.h"
#include "Sail/graphics/camera/CameraController.h"
#include "Sail/entities/components/Components.h"
#include "../../../api/Input.h"
#include "Sail/KeyBinds.h"
#include "Sail/Application.h"
#include "Sail/utils/GameDataTracker.h"
#include "../../ECS.h"
#include "../physics/UpdateBoundingBoxSystem.h"
#include "Sail/entities/components/LocalOwnerComponent.h"
#include "Sail/entities/components/AudioComponent.h"
#include "../src/Network/NWrapperSingleton.h"
#include "Sail/entities/components/MapComponent.h"

#include "Sail/TimeSettings.h"


// Candle can only be picked up and put down once every 0.2 seconds
constexpr float CANDLE_TIMER = 0.2f;


GameInputSystem::GameInputSystem() : BaseComponentSystem() {
	registerComponent<LocalOwnerComponent>(true, true, false);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<SpeedLimitComponent>(true, true, false);
	registerComponent<CollisionComponent>(true, true, false);
	registerComponent<AudioComponent>(true, true, true);
	registerComponent<BoundingBoxComponent>(true, true, false);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<CandleComponent>(false, true, true);
	registerComponent<GunComponent>(false, true, true);

	// cam variables
	m_yaw = 160.f;
	m_pitch = 0.f;
	m_roll = 0.f;

	m_gameDataTracker = &GameDataTracker::getInstance();
}

GameInputSystem::~GameInputSystem() {
	clean();
}

void GameInputSystem::fixedUpdate(float dt) {
	this->processKeyboardInput(dt);
}

void GameInputSystem::update(float dt, float alpha) {
	this->processMouseInput(dt);
	this->updateCameraPosition(alpha);
}

void GameInputSystem::initialize(Camera* cam) {
	if ( m_cam == nullptr ) {
		m_cam = SAIL_NEW CameraController(cam);
	} else {
		CameraController* tempCam = m_cam;
		Memory::SafeDelete(tempCam);
		m_cam = SAIL_NEW CameraController(cam);
	}
}

void GameInputSystem::clean() {
	Memory::SafeDelete(m_cam);
}

void GameInputSystem::stop() {
	clean();
}

void GameInputSystem::processKeyboardInput(const float& dt) {
	m_candleToggleTimer += dt;

	for ( auto e : entities ) {
		// Get player movement inputs
		Movement playerMovement = getPlayerMovementInput(e);

		// Calculate forward vector for player
		glm::vec3 forward = m_cam->getCameraDirection();

		// Do flying movement if in spectator mode
		if ( e->hasComponent<SpectatorComponent>() ) {
			if ( playerMovement.forwardMovement != 0.f || playerMovement.rightMovement != 0.f || playerMovement.upMovement != 0.f ) {
 				forward = glm::normalize(forward);

				// Calculate right vector for player
				glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
				right = glm::normalize(right);

				glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

				auto transComp = e->getComponent<TransformComponent>();
				float speed = 5.f;
				glm::vec3 moveDir = ( playerMovement.forwardMovement * forward + playerMovement.rightMovement * right + playerMovement.upMovement * up );
				moveDir = glm::normalize(moveDir);
				transComp->translate(moveDir * dt * playerMovement.speedModifier * 10.f);
			}

			// Else do normal movement
		} else {
			auto collision = e->getComponent<CollisionComponent>();
			auto movement = e->getComponent<MovementComponent>();
			auto speedLimit = e->getComponent<SpeedLimitComponent>();
			auto audioComp = e->getComponent<AudioComponent>();

			// Get player movement inputs
			Movement playerMovement = getPlayerMovementInput(e);

			// Player puts down candle
			if (Input::IsKeyPressed(KeyBinds::TOGGLE_CANDLE_HELD) ) {
				if (m_candleToggleTimer > CANDLE_TIMER) {
					putDownCandle(e);
					m_candleToggleTimer = 0.0f;
				}
			}

			if ( Input::WasKeyJustPressed(KeyBinds::LIGHT_CANDLE) ) {
				for ( auto child : e->getChildEntities() ) {
					if ( child->hasComponent<CandleComponent>() ) {
						auto candle = child->getComponent<CandleComponent>();
						if (!candle->isLit) {
							candle->userReignition = true;
						}
					}
				}
			}

			if (collision->onGround) {
				if (m_fallTimer > m_fallThreshold) {
					// Send event to play the sound for the landing (will be sent to ourself too)
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_LANDED,
						SAIL_NEW Netcode::MessagePlayerLanded{ e->getComponent<NetworkSenderComponent>()->m_id }
					);
				}
				else if (m_fallTimer > 0.0f) {

					m_onGroundTimer = m_onGroundThreshold;
				}
				m_fallTimer = 0.0f;

				if (m_onGroundTimer >= m_onGroundThreshold) {
					m_isPlayingRunningSound = true;
					m_onGroundTimer = m_onGroundThreshold;

				} else {

					m_onGroundTimer += dt;
				}
			}
			// NOTE: Jumping sets m_onGroundTimer to -1.0f to stop running sound right away.
			//		 Needed for when running normally to avoid 'stuttering' effect from playing
			//		 the sound multiple times within a short time span.
			else if (!collision->onGround) {
				if (m_onGroundTimer < 0.0f) {

					m_onGroundTimer = 0.0f;
					m_isPlayingRunningSound = false;
				} else {
					m_fallTimer += dt;
					m_onGroundTimer -= dt;
				}
			}

			if ( playerMovement.upMovement == 1.0f ) {

				if (!m_wasSpacePressed && collision->onGround) {

					movement->velocity.y = 5.0f;
					// AUDIO TESTING - JUMPING
					m_onGroundTimer = -1.0f; // To stop walking sound immediately when jumping
					m_gameDataTracker->logJump();

					// Send event to play the sound for the jump (will be sent to ourself too)
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_JUMPED,
						SAIL_NEW Netcode::MessagePlayerJumped{ e->getComponent<NetworkSenderComponent>()->m_id }
					);
				}
				m_wasSpacePressed = true;
			} else if (m_wasSpacePressed){
				m_wasSpacePressed = false;
			}

			forward.y = 0.f;
			forward = glm::normalize(forward);
			// Calculate right vector for player
			glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
			right = glm::normalize(right);


			// calculate forward vel and sidevel

			movement->relVel.x = glm::dot(movement->velocity, forward);
			movement->relVel.z = glm::dot(movement->velocity, right);
			movement->relVel.y = glm::dot(movement->velocity, glm::vec3(0.f, 1.f, 0.f));

			// Prevent division by zero
			if ( playerMovement.forwardMovement != 0.0f || playerMovement.rightMovement != 0.0f ) {

				// Calculate total movement
				float acceleration = 70.0f - ( glm::length(movement->velocity) / speedLimit->maxSpeed ) * 20.0f;
				if ( !collision->onGround ) {
					acceleration = acceleration * 0.5f;
					// AUDIO TESTING (turn OFF looping running sound)
					if (!m_isPlayingRunningSound) {
						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						if (!tempStopAll) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_STOP_SOUND,
								SAIL_NEW Netcode::MessageRunningStopSound{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = true;
							tempMetal = false;
							tempTile = false;
						}
					}
				}
				// AUDIO TESTING (playing a looping running sound)
				else if ( m_runSoundTimer > m_onGroundThreshold) {
					// CORRIDOR
					if (m_mapPointer->getAreaType(e->getComponent<TransformComponent>()->getTranslation().x, e->getComponent<TransformComponent>()->getTranslation().z) == 0) {
						
						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						if (!tempMetal) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_METAL_START,
								SAIL_NEW Netcode::MessageRunningMetalStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = false;
							tempMetal = true;
							tempTile = false;
						}
					}
					// ROOM
					else /*(AreaType > 0)*/ {
						// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
						if (!tempTile) {
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::RUNNING_TILE_START,
								SAIL_NEW Netcode::MessageRunningTileStart{ e->getComponent<NetworkSenderComponent>()->m_id }
							);

							tempStopAll = false;
							tempMetal = false;
							tempTile = true;
						}
					}
				} else {

					m_runSoundTimer += dt;
				}

				movement->accelerationToAdd =
					glm::normalize(right * playerMovement.rightMovement + forward * playerMovement.forwardMovement)
					* acceleration;



			} else {

				// AUDIO TESTING (turn OFF looping running sound)
				// If-statement and relevant bools are to avoid sending unnecessary amount of messages/data
				if (!tempStopAll) {
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::RUNNING_STOP_SOUND,
						SAIL_NEW Netcode::MessageRunningStopSound{ e->getComponent<NetworkSenderComponent>()->m_id }
					);

					tempStopAll = true;
					tempMetal = false;
					tempTile = false;
				}
				m_runSoundTimer = 0.0f;
			}
		}
	}
}

void GameInputSystem::processMouseInput(const float& dt) {
	// Toggle cursor capture on right click
	for (auto e : entities) {

//#ifdef DEVELOPMENT
		if (Input::WasMouseButtonJustPressed(KeyBinds::DISABLE_CURSOR)) {
			Input::HideCursor(!Input::IsCursorHidden());
		}
//#endif
		
		auto trans = e->getComponent<TransformComponent>();
		auto rots = trans->getRotations();
		m_pitch = (rots.z != 0.f) ? glm::degrees(-rots.z) : m_pitch;
		m_yaw = glm::degrees(-rots.y);

		// Update pitch & yaw if window has focus
		if (Input::IsCursorHidden()) {
			glm::ivec2& mouseDelta = Input::GetMouseDelta();
			m_pitch -= mouseDelta.y * m_lookSensitivityMouse;
			m_yaw -= mouseDelta.x * m_lookSensitivityMouse;
		}

		// Lock pitch to the range -89 - 89
		if (m_pitch >= 89) {
			m_pitch = 89;
		}
		else if (m_pitch <= -89) {
			m_pitch = -89;
		}

		// Lock yaw to the range 0 - 360
		if (m_yaw >= 360) {
			m_yaw -= 360;
		}
		else if (m_yaw <= 0) {
			m_yaw += 360;
		}

		trans->setRotations(0.f, glm::radians(-m_yaw), 0.f);

		GunComponent* gc = e->getComponent<GunComponent>();
		TransformComponent* ptc = e->getComponent<TransformComponent>();
		if (gc) {
			for (auto childE : e->getChildEntities()) {
				if (childE->getName().find("WaterGun") != std::string::npos) {
					TransformComponent* tc = childE->getComponent<TransformComponent>();
					tc->setRotations(glm::radians(-m_pitch),0, 0);
				}
			}
		}

		if (!e->hasComponent<SpectatorComponent>() && Input::IsMouseButtonPressed(KeyBinds::SHOOT)) {
			GunComponent* gc = e->getComponent<GunComponent>();
			TransformComponent* ptc = e->getComponent<TransformComponent>();
			if (gc) {
				for (auto childE : e->getChildEntities()) {
					if (childE->getName().find("WaterGun") != std::string::npos) {
						TransformComponent* tc = childE->getComponent<TransformComponent>();
						glm::vec3 gunPosition = glm::vec3(tc->getMatrixWithUpdate()[3]) + m_cam->getCameraDirection() * 0.33f;

						e->getComponent<GunComponent>()->setFiring(gunPosition, m_cam->getCameraDirection());
					}
				}
			}
		} else {

			GunComponent* gc = e->getComponent<GunComponent>();
			if (gc) {
				gc->firing = false;
			}
		}
	}
}


void GameInputSystem::updateCameraPosition(float alpha) {
	for ( auto e : entities ) {
		TransformComponent* playerTrans = e->getComponent<TransformComponent>();
		BoundingBoxComponent* playerBB = e->getComponent<BoundingBoxComponent>();

		glm::vec3 forwards(
			std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw + 90)),
			std::sin(glm::radians(m_pitch)),
			std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw + 90))
		);
		forwards = glm::normalize(forwards);

		playerTrans->setRotations(0.f, glm::radians(-m_yaw), 0.f);

		m_cam->setCameraPosition(glm::vec3(playerTrans->getInterpolatedTranslation(alpha) + glm::vec3(0.f, playerBB->getBoundingBox()->getHalfSize().y * 1.8f, 0.f)));
		m_cam->setCameraDirection(forwards);
	}
}

CameraController* GameInputSystem::getCamera() const {
	return m_cam;
}

void GameInputSystem::putDownCandle(Entity* e) {
	for ( int i = 0; i < e->getChildEntities().size(); i++ ) {
		auto candleE = e->getChildEntities()[i];
		if ( candleE->hasComponent<CandleComponent>() ) {
			auto candleComp = candleE->getComponent<CandleComponent>();
			candleComp->isCarried = !candleComp->isCarried;
			return;
		}
	}
}

Movement GameInputSystem::getPlayerMovementInput(Entity* e) {
	Movement playerMovement;

	if ( Input::IsKeyPressed(KeyBinds::SPRINT) ) { playerMovement.speedModifier = m_runSpeed; }

	if ( Input::IsKeyPressed(KeyBinds::MOVE_FORWARD) ) { playerMovement.forwardMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::MOVE_BACKWARD) ) { playerMovement.forwardMovement -= 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::MOVE_LEFT) ) { playerMovement.rightMovement -= 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::MOVE_RIGHT) ) { playerMovement.rightMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::MOVE_UP) ) { playerMovement.upMovement += 1.0f; }
	if ( Input::IsKeyPressed(KeyBinds::MOVE_DOWN) ) { playerMovement.upMovement -= 1.0f; }

	return playerMovement;
}