#include "pch.h"

#include "GunSystem.h"


#include "Sail/entities/ECS.h"

#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/NetworkSenderComponent.h"
#include "Sail/entities/components/MovementComponent.h"

#include "Sail/utils/GameDataTracker.h"
#include "../Sail/src/Network/NWrapperSingleton.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <random>


// TODO: Remove, here only for temporary debugging
#include <iostream>

GunSystem::GunSystem() : BaseComponentSystem() {
	// TODO: System owner should check if this is correct
	registerComponent<GunComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, false);
	registerComponent<NetworkSenderComponent>(false, true, true);

	m_gameDataTracker = &GameDataTracker::getInstance();
}

GunSystem::~GunSystem() {

}


void GunSystem::update(float dt) {
	for (auto& e : entities) {
		GunComponent* gun = e->getComponent<GunComponent>();

		// Gun is firing
		if (gun->firing) {

			// Gun is not overloaded
			if (gun->gunOverloadTimer <= 0) {

				// SHOOT
				if (gun->projectileSpawnTimer <= 0.f) {

					// Determine projectileSpeed based on how long the gun has been firing continuously
					alterProjectileSpeed(gun);

					// Tell yours and everybody else's NetworkReceiverSystem to spawn the projectile
					for (int i = 0; i < 2; i++) {
						NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
							Netcode::MessageType::SPAWN_PROJECTILE,
							SAIL_NEW Netcode::MessageSpawnProjectile{
								gun->position,
								gun->direction * gun->projectileSpeed + e->getComponent<MovementComponent>()->velocity,
								e->getComponent<NetworkSenderComponent>()->m_id
							}
						);
					}
					m_gameDataTracker->logWeaponFired();
					fireGun(e, gun);
				}
				// DO NOT SHOOT (Cooldown between shots)
				else {
					gun->firingContinuously = false;
				}

				// Overload the gun if necessary
				if ((gun->gunOverloadvalue += dt) > gun->gunOverloadThreshold) {
					overloadGun(e, gun);
				}
			}
		}
		// Gun is not firing.
		else {
			// Reduce the overload value
			if (gun->gunOverloadvalue > 0) {
				gun->gunOverloadvalue -= dt;
			}

			// Gun stopped firing
			if (gun->firingContinuously) {
				// Set gun state to END
				setGunStateEND(e, gun);
			}

			gun->firingContinuously = false;
		}

		// Play shooting sounds based on the GunState
		playShootingSounds(e);

		gun->gunOverloadTimer -= dt;
		gun->projectileSpawnTimer -= dt;
	}
}


void GunSystem::alterProjectileSpeed(GunComponent* gun) {
	gun->projectileSpeed = gun->baseProjectileSpeed + (gun->projectileSpeedRange * (gun->gunOverloadvalue/gun->gunOverloadThreshold));
}

void GunSystem::fireGun(Entity* e, GunComponent* gun) {
	gun->projectileSpawnTimer = gun->m_projectileSpawnCooldown;

	// If this is the first shot in this "burst" of projectiles...
	if (!gun->firingContinuously) {
		setGunStateSTART(e, gun);
	}
	else {
		setGunStateLOOP(e, gun);
	}

	gun->firingContinuously = true;

	// Update the frequency of the lowpass based on guns current overload percentage
	float frequency = 9000 - (9000 - 2000) * (gun->gunOverloadvalue / gun->gunOverloadThreshold);
	if (e->hasComponent<AudioComponent>()) {
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_START].frequency = frequency;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_LOOP].frequency = frequency;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SHOOT_END].frequency = frequency;
	}
}

void GunSystem::overloadGun(Entity* e, GunComponent* gun) {
	// Don't overload the gun at all if the cooldown is '0'
	if (gun->m_gunOverloadCooldown == 0.0f) {
		gun->gunOverloadvalue = gun->gunOverloadThreshold;
		return;
	} 

	gun->gunOverloadvalue = 0;
	gun->gunOverloadTimer = 0;

	// If we have some sort of cooldown, behave statewise as usual
	setGunStateEND(e, gun);
	gun->firingContinuously = false;	
}

void GunSystem::setGunStateSTART(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addMessageType(
		Netcode::MessageType::SHOOT_START
	);

	gun->state = GunState::STARTING;
}

void GunSystem::setGunStateLOOP(Entity* e, GunComponent* gun) {
	// Network automatically handles transition from starting-->looping
	gun->state = GunState::LOOPING;
}

void GunSystem::setGunStateEND(Entity* e, GunComponent* gun) {
	e->getComponent<NetworkSenderComponent>()->addMessageType(
		Netcode::MessageType::SHOOT_END
	);
	gun->state = GunState::ENDING;
}

void GunSystem::playShootingSounds(Entity* e) {
	GunComponent* gun = e->getComponent<GunComponent>();

	// Play sounds depending on which state the gun is in.
	if (gun->state == GunState::STARTING) {
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = true;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].playOnce = true;
	}
	else if (gun->state == GunState::LOOPING) {
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_START].isPlaying = false;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = true;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].playOnce = true;
	}
	else if (gun->state == GunState::ENDING) {
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_LOOP].isPlaying = false;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].isPlaying = true;
		e->getComponent<AudioComponent>()->m_sounds[Audio::SoundType::SHOOT_END].playOnce = true;

		gun->state = GunState::STANDBY;
	}
	else {	/* gun->state == GunState::STANDBY */

	}
}
