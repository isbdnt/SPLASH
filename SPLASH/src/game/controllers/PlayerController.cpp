#include "pch.h"
#include "PlayerController.h"
#include "Sail.h"

PlayerController::PlayerController(Camera* cam, Scene* scene) {
	m_cam = SAIL_NEW CameraController(cam);
	m_scene = scene;
	m_player = ECS::Instance()->createEntity("player_entity");

	//m_player->addComponent<MovementComponent>(/*initialSpeed*/ 0.f, /*initialDirection*/ m_cam->getCameraDirection());
	m_player->addComponent<TransformComponent>(m_cam->getCameraPosition());
	m_player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(0.0f, 0.f, 0.f));

	m_yaw = 90.f;
	m_pitch = 0.f;
	m_roll = 0.f;
}

PlayerController::~PlayerController() {
	delete m_cam;
	m_projectiles.clear();
}

void PlayerController::setStartPosition(const glm::vec3& pos) {

}

// To be run at the beginning of each update tick
void PlayerController::prepareUpdate() {
	TransformComponent* transform = m_player->getComponent<TransformComponent>();
	if (transform) { transform->prepareUpdate(); }
}

void PlayerController::processKeyboardInput(float dt) {
	float speedModifier = 1.f;
	float forwardMovement = 0.0f;
	float rightMovement = 0.0f;
	float upMovement = 0.0f;

	PhysicsComponent* physicsComp = m_player->getComponent<PhysicsComponent>();

	float tempY = physicsComp->velocity.y;

	// Increase speed if shift or right trigger is pressed
	if (Input::IsKeyPressed(KeyBinds::sprint)) { speedModifier = RUN_SPEED; }

	if (Input::IsKeyPressed(KeyBinds::moveForward)) { forwardMovement += 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::moveBackward)) { forwardMovement -= 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::moveLeft)) { rightMovement -= 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::moveRight)) { rightMovement += 1.0f; }
	if (Input::IsKeyPressed(KeyBinds::moveUp)) {
		if (!m_wasSpacePressed) {
			tempY = 15.0f;
		}
		m_wasSpacePressed = true;
	}
	else {
		m_wasSpacePressed = false;
	}
	//if (Input::IsKeyPressed(KeyBinds::moveDown)) { upMovement -= 1.0f; }


	glm::vec3 forwards(
		std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw)),
		std::sin(glm::radians(m_pitch)),
		std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw))
	);
	forwards = glm::normalize(forwards);

	glm::vec3 forward = m_cam->getCameraDirection();
	forward.y = 0.f;
	forward = glm::normalize(forward);

	glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
	right = glm::normalize(right);

	TransformComponent* playerTrans = m_player->getComponent<TransformComponent>();

	// the player's transform will be modified in the physicSystem so save the player's current
	// position for interpolation later.
	playerTrans->prepareUpdate();

	// Prevent division by zero
	if (forwardMovement != 0.0f || rightMovement != 0.0f || upMovement != 0.0f) {
		// Calculate total movement
		physicsComp->velocity =
			glm::normalize(right * rightMovement + forward * forwardMovement)
			* m_movementSpeed * speedModifier;
	}
	else {
		physicsComp->velocity = glm::vec3(0.0f);
	}

	physicsComp->velocity.y = tempY;

	// Shooting

	// Shoot gun
	// TODO: This should probably be moved elsewhere.
	//       See if it should be done every tick or every frame and where the projectiles are to be created
	if (Input::IsMouseButtonPressed(0)) {
		// TODO: add and tweak guncomponent+projectile system once playercontroller is changed to a system
		glm::vec3 camRight = glm::cross(m_cam->getCameraUp(), m_cam->getCameraDirection());
		glm::vec3 gunPosition = m_cam->getCameraPosition() + (m_cam->getCameraDirection() + camRight - m_cam->getCameraUp());
		m_player->getComponent<GunComponent>()->setFiring(gunPosition, m_cam->getCameraDirection());
	}
}

void PlayerController::processMouseInput(float dt) {
	PhysicsComponent* physicsComp = m_player->getComponent<PhysicsComponent>();

	// Mouse input

	// Toggle cursor capture on right click
	if (Input::WasMouseButtonJustPressed(SAIL_MOUSE_RIGHT_BUTTON)) {
		Input::HideCursor(!Input::IsCursorHidden());
	}

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
}

void PlayerController::updateCameraPosition(float alpha) {
	TransformComponent* playerTrans = m_player->getComponent<TransformComponent>();
	BoundingBoxComponent* playerBB = m_player->getComponent<BoundingBoxComponent>();

	glm::vec3 forwards(
		std::cos(glm::radians(m_pitch)) * std::cos(glm::radians(m_yaw)),
		std::sin(glm::radians(m_pitch)),
		std::cos(glm::radians(m_pitch)) * std::sin(glm::radians(m_yaw))
	);
	forwards = glm::normalize(forwards);
	//playerTrans->setForward(forwards); //needed?


	m_cam->setCameraPosition(glm::vec3(playerTrans->getInterpolatedTranslation(alpha) + glm::vec3(0.f,playerBB->getBoundingBox()->getHalfSize().y*0.8f,0.f)));
	m_cam->setCameraDirection(forwards);

	//moves the candlemodel and its pointlight to the correct position and rotates it to not spin when the player turns
	glm::vec3 forward = m_cam->getCameraDirection();
	forward.y = 0.f;
	forward = glm::normalize(forward);

	glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
	right = glm::normalize(right);
	glm::vec3 playerToCandle = glm::vec3((forward - right)*0.2f);
	glm::vec3 candlePos = m_cam->getCameraPosition() + playerToCandle - glm::vec3(0, 0.35f, 0);
	m_candle->getComponent<TransformComponent>()->setTranslation(candlePos);
	glm::vec3 candleRot = glm::vec3(0.f, glm::radians(-m_yaw), 0.f);
	m_candle->getComponent<TransformComponent>()->setRotations(candleRot);
	m_candle->getComponent<TransformComponent>()->prepareUpdate();
	glm::vec3 flamePos = candlePos + glm::vec3(0, 0.37f, 0);
	glm::vec3 plPos = flamePos - playerToCandle * 0.1f;
	m_candle->getComponent<LightComponent>()->getPointLight().setPosition(plPos);
}

void PlayerController::destroyOldProjectiles() {
	// Remove old projectiles
	/*for (int i = 0; i < m_projectiles.size(); i++) {
		if (m_projectiles[i].projectile->isAboutToBeDestroyed()) {
			ECS::Instance()->destroyEntity(m_projectiles[i].projectile);
			m_projectiles.erase(m_projectiles.begin() + i);
			i--;
		}
	}*/
}

// NOTE: Keyboard and mouse input processing has been moved to their own functions above this one
void PlayerController::update(float dt) {
	/*for (int i = 0; i < m_projectiles.size(); i++) {
		for (unsigned int j = 0; j < m_candles->size(); j++) {
			auto collisions = m_projectiles[i].projectile->getComponent<PhysicsComponent>()->collisions;
			for (unsigned int k = 0; k < collisions.size(); k++) {
				if (collisions[k].entity == m_candles->at(j).get()) {
					m_candles->at(j)->removeComponent<LightComponent>();
				}
			}
		}
	}*/
}

std::shared_ptr<Entity> PlayerController::getEntity() {
	return m_player;
}

void PlayerController::setProjectileModels(Model* model, Model* wireframeModel) {
	m_projectileModel = model;
	m_projectileWireframeModel = wireframeModel;
	m_player->addComponent<GunComponent>(m_projectileModel);
}

void PlayerController::provideCandles(std::vector<Entity::SPtr>* candles) {
	m_candles = candles;
}

std::shared_ptr<Entity> PlayerController::getCandle() {
	return m_candle;
}

//creates and binds the candle model and a pointlight for the player.
void PlayerController::createCandle(Model* model) {
	auto e = ECS::Instance()->createEntity("PlayerCandle");//;//ECS::Instance()->createEntity("PlayerCandle");
	e->addComponent<ModelComponent>(model);
	glm::vec3 camRight = glm::cross(m_cam->getCameraUp(), m_cam->getCameraDirection());
	//camRight = glm::normalize(camRight);
	glm::vec3 candlePos = -m_cam->getCameraDirection() + camRight;// -m_cam->getCameraUp();
	e->addComponent<TransformComponent>(candlePos);// , m_player->getComponent<TransformComponent>());
	//e->addComponent<TransformComponent>(glm::vec3(-1.f, -3.f, 1.f), m_player->getComponent<TransformComponent>());
	//e->getComponent<TransformComponent>()->setParent(m_player->getComponent<TransformComponent>());
	m_candle = e;
	PointLight pl;
	glm::vec3 lightPos = e->getComponent<TransformComponent>()->getTranslation();
	pl.setColor(glm::vec3(0.5f, 0.5f, 0.5f));
	pl.setPosition(glm::vec3(lightPos.x, lightPos.y + 3.1f, lightPos.z));
	//pl.setAttenuation(.0f, 0.1f, 0.02f);
	pl.setIndex(2);
	e->addComponent<LightComponent>(pl);
	m_scene->setPlayerCandle(e);

}
