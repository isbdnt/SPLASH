#include "pch.h"
#include "CandleSystem.h"

#include "Sail/entities/components/LightComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/PhysicsComponent.h"
#include "Sail/entities/components/TransformComponent.h"

#include "Sail/graphics/camera/CameraController.h"

#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/physics/UpdateBoundingBoxSystem.h"

#include "Sail/Application.h"

CandleSystem::CandleSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<CandleComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
	registerComponent<LightComponent>(true, true, true);
}

CandleSystem::~CandleSystem() {

}

void CandleSystem::setPlayerEntityID(int entityID) {
	m_playerEntityID = entityID;
}

// turn on the light of a specified candle if it doesn't have one already
void CandleSystem::lightCandle(const std::string& name) {
	for (auto e : entities) {
		if (e->getName() == name) {
			e->getComponent<LightComponent>()->getPointLight().setColor(glm::vec3(1.0f, 1.0f, 1.0f));
			break;
		}
	}
}

// should be updated after collision detection has been done
void CandleSystem::update(float dt) {
	for (auto e : entities) {

		auto candle = e->getComponent<CandleComponent>();



		
		if ( candle->getIsAlive() ) {
			// Remove light from candles that were hit by projectiles
			if ( candle->wasHitByWater() ) {
				candle->resetHitByWater();
				e->getComponent<LightComponent>()->getPointLight().setColor(glm::vec3(0.0f, 0.0f, 0.0f));
				candle->setIsLit(false);

				if ( candle->getOwner() == m_playerEntityID ) {
					if ( !candle->isCarried() ) {
						candle->toggleCarried();
					}
				}

				if ( candle->getNumRespawns() == m_maxNumRespawns ) {
					candle->setIsAlive(false);

					// Check if the extinguished candle is owned by the player
					// If so, dispatch an event (received by GameState for now)
					if ( candle->getOwner() == m_playerEntityID ) {
						Application::getInstance()->dispatchEvent(Event(Event::Type::PLAYER_CANDLE_HIT));
					}
				}

			} else if ( candle->getDoActivate() || candle->getDownTime() >= m_candleForceRespawnTimer /* Relight the candle every 5 seconds (should probably be removed later) */ ) {
				e->getComponent<LightComponent>()->getPointLight().setColor(glm::vec3(0.3f, 0.3f, 0.3f));
				candle->setIsLit(true);
				candle->incrementRespawns();
				candle->resetDownTime();
				candle->resetDoActivate();
			} else if ( !candle->getIsLit() ) {
				candle->addToDownTime(dt);
			}

			if ( candle->isCarried() != candle->getWasCarriedLastUpdate() ) {
				putDownCandle(e);
			}

			candle->setWasCarriedLastUpdate(candle->isCarried());
			glm::vec3 flamePos = glm::vec3(e->getComponent<TransformComponent>()->getMatrix()[3]) + glm::vec3(0, 0.5f, 0);
			e->getComponent<LightComponent>()->getPointLight().setPosition(flamePos);
		}
	}
}

void CandleSystem::putDownCandle(Entity* e) {
	auto candleComp = e->getComponent<CandleComponent>();

	auto candleTransComp = e->getComponent<TransformComponent>();
	auto parentTransComp = e->getParent()->getComponent<TransformComponent>();
	/* TODO: Raycast and see if the hit location is ground within x units */
	if ( !candleComp->isCarried() ) {
		candleTransComp->removeParent();
		glm::vec3 dir = glm::vec3(1.0f, 0.f, 1.0f);// TODO: parentTransComp->getForward()
		candleTransComp->setTranslation(parentTransComp->getTranslation() + dir);
		ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.0f);
	} else if ( candleComp->isCarried() ) {
		if ( glm::length(parentTransComp->getTranslation() - candleTransComp->getTranslation()) < 2.0f || !candleComp->getIsLit() ) {
			candleTransComp->setTranslation(glm::vec3(0.f, 2.0f, 0.f));
			candleTransComp->setParent(parentTransComp);
		} else {
			candleComp->toggleCarried();
		}
	}
}