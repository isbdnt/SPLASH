#include "pch.h"

#include "GunSystem.h"

#include "Sail/entities/ECS.h"

#include "Sail/entities/components/ProjectileComponent.h"
#include "Sail/entities/components/LifeTimeComponent.h"
#include "Sail/entities/components/BoundingBoxComponent.h"
#include "Sail/entities/components/ModelComponent.h"
#include "Sail/entities/components/MovementComponent.h"
#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/TransformComponent.h"
#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/CollisionComponent.h"

#include "Sail/entities/components/MetaballComponent.h"
#include "Sail/utils/GameDataTracker.h"
#include "Sail/entities/components/CollidableComponent.h"

#include <random>

GunSystem::GunSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<GunComponent>(true, true, true);
	m_gameDataTracker = &GameDataTracker::getInstance();
}

GunSystem::~GunSystem() {

}


void GunSystem::update(float dt) {
	for (auto& e : entities) {
		GunComponent* gun = e->getComponent<GunComponent>();

		if (gun->firing) {
			if (gun->gunOverloadTimer <= 0) {
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					gun->gunOverloadTimer = gun->m_gunOverloadCooldown;
					gun->gunOverloadvalue = 0;
				}

				if (gun->projectileSpawnTimer <= 0.f) {
					gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

					for (int i = 0; i <= 1; i++) {
						// Create entity and attach necessary components etc
						Entity* e = GunFactory::createWaterBullet(
							gun->position,
							gun->direction,
							gun->projectileSpeed,
							i,
							true
						);

						// Let network know that a projectile was spawned.
						unsigned char id = e->getID();
						e->addComponent<NetworkSenderComponent>(
							Netcode::MessageType::SPAWN_PROJECTILE,
							Netcode::EntityType::PLAYER_ENTITY,
							id
						);

						// Log it for stats.
						m_gameDataTracker->logWeaponFired();
					}
				}
			}

			gun->firing = false;
		}
		else {
			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;
			}
		}

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}

}

Entity* GunFactory::createWaterBullet(glm::vec3 pos, glm::vec3 dir, float projSpeed, int i, bool ownedLocally) {
	auto e = ECS::Instance()->createEntity("projectile");
	
	glm::vec3 randPos;
	float maxrand = 0.2f;

	//Will remove rand later.
	randPos.r = ((float)rand() / RAND_MAX) * maxrand;
	randPos.g = ((float)rand() / RAND_MAX) * maxrand;
	randPos.b = ((float)rand() / RAND_MAX) * maxrand;


	e->addComponent<MetaballComponent>();
	e->addComponent<BoundingBoxComponent>();
	e->getComponent<BoundingBoxComponent>()->getBoundingBox()->setHalfSize(glm::vec3(0.1, 0.1, 0.1));
	e->addComponent<LifeTimeComponent>(4.0f);
	e->addComponent<ProjectileComponent>(10.0f, ownedLocally);
	e->addComponent<TransformComponent>((pos + randPos) - dir * (0.15f * i));

	MovementComponent* movement = e->addComponent<MovementComponent>();
	movement->velocity = dir * projSpeed;
	movement->constantAcceleration = glm::vec3(0.f, -9.8f, 0.f);

	CollisionComponent* collision = e->addComponent<CollisionComponent>();
	collision->drag = 2.0f;
	// NOTE: 0.0f <= Bounciness <= 1.0f
	collision->bounciness = 0.1f;
	collision->padding = 0.16f;

	
	return e.get();
}


