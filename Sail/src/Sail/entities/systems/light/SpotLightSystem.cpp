#include "pch.h"
#include "SpotLightSystem.h"

#include "Sail/entities/components/SpotlightComponent.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/entities/ECS.h"
#include "Sail/graphics/light/LightSetup.h"

SpotLightSystem::SpotLightSystem() : BaseComponentSystem() {
	registerComponent<SpotlightComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, false);
}

SpotLightSystem::~SpotLightSystem() {
}


//check and update all lights for all entities
void SpotLightSystem::updateLights(LightSetup* lightSetup, float alpha, float dt) {
	int id = 0;
	lightSetup->getSLs().clear();
	for (auto e : entities) {
		SpotlightComponent* sc = e->getComponent<SpotlightComponent>();
		MovementComponent* mc = e->getComponent<MovementComponent>();

		mc->rotation.y = 0.f;

		// Update active lights
		if (!sc->isOn) {
			continue;
		}

		mc->rotation.y = 4.f;

		TransformComponent* t = e->getComponent<TransformComponent>();


		sc->m_lightEntityRotated = sc->light;
		sc->m_lightEntityRotated.setIndex(id++);
		sc->m_lightEntityRotated.setDirection(glm::rotate(t->getInterpolatedRotation(alpha), sc->light.getDirection()));
		sc->m_lightEntityRotated.setPosition(sc->light.getPosition() + t->getInterpolatedTranslation(alpha));
		
		lightSetup->addSpotLight(sc->m_lightEntityRotated);
	}
}

void SpotLightSystem::toggleONOFF() {
	for (auto e : entities) {
		SpotlightComponent* sc = e->getComponent<SpotlightComponent>();
		sc->isOn = !sc->isOn;
	}
}

void SpotLightSystem::enableHazardLights(std::vector<int> activeRooms) {
	for (auto e : entities) {
		SpotlightComponent* sc = e->getComponent<SpotlightComponent>();
		std::vector<int>::iterator it = std::find(activeRooms.begin(), activeRooms.end(), sc->roomID);

		sc->isOn = (it != activeRooms.end());
	}
}