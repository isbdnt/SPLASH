#include "pch.h" 
#include "PowerUpCollectibleSystem.h"
#include "Sail/entities/components/PowerUp/PowerUpCollectibleComponent.h"
#include "Network/NWrapperSingleton.h"

PowerUpCollectibleSystem::PowerUpCollectibleSystem() :
	m_collectDistance(2.0f),
	m_playerList(nullptr)
{
	registerComponent<PowerUpCollectibleComponent>(true, true, true);

	EventDispatcher::Instance().subscribe(Event::Type::SPAWN_POWERUP, this);
	EventDispatcher::Instance().subscribe(Event::Type::DESTROY_POWERUP, this);
}

PowerUpCollectibleSystem::~PowerUpCollectibleSystem() {
	EventDispatcher::Instance().unsubscribe(Event::Type::SPAWN_POWERUP, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::DESTROY_POWERUP, this);
}

void PowerUpCollectibleSystem::init(std::vector<Entity*>* playerList) {
	m_playerList = playerList;
}

void PowerUpCollectibleSystem::update(float dt) {

	const bool isHost = NWrapperSingleton::getInstance().isHost();

	m_distances.clear();
	//Only let host control powerup pickup for all players.
	if (!isHost) {
		return;
	}
	if (!m_playerList) {
		return;
	}

	updateSpawns(dt);

	for (auto& e : entities) {
		auto* transformC = e->getComponent<TransformComponent>();
		auto* powerCC = e->getComponent<PowerUpCollectibleComponent>();
		if (transformC && powerCC) {
			if (powerCC->time > 0.0f) {
				continue;
			}
			
			// CHECK AGAINST PLAYERS
			for (auto* player : *m_playerList) {
				if (auto * playerTC = player->getComponent<TransformComponent>()) {
					float dist = glm::distance(transformC->getTranslation(), playerTC->getTranslation());
					m_distances.emplace_back(dist);
					if (dist <= m_collectDistance) {
						if (auto* playerpowerC = player->getComponent<PowerUpComponent>()) {
							
							//Send to both clients and host
							NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
								Netcode::MessageType::DESTROY_POWER_UP,
								SAIL_NEW Netcode::MessageDestroyPowerUp{
									e->getComponent<NetworkReceiverComponent>()->m_id,
									player->getComponent<NetworkReceiverComponent>()->m_id
								}
								, false
							);

							e->queueDestruction();
							playerpowerC->powerUps[powerCC->powerUp].addTime(powerCC->powerUpDuration);

							if (powerCC->respawnTime > 0.0f) {
								//powerCC->time = powerCC->respawnTime; // TODO: CHANGE TO NETWORK MESSAGE
								m_respawns.push_back({ 5, PowerUps(powerCC->powerUp), transformC->getTranslation() });
							}
						}
					}
				}
			}
		}
	}
}
void PowerUpCollectibleSystem::spawnSingleUsePowerUp(const PowerUps powerUp, const float time, glm::vec3 pos, Entity* parent) {
	Entity::SPtr e = EntityFactory::CreatePowerUp(pos, powerUp);
	auto* pC = e->getComponent<PowerUpCollectibleComponent>();
	pC->respawnTime = -1.0f;
	pC->powerUpDuration = time;
	if (parent) {



	}
	//TODO: SEND MESSAGE
}
void PowerUpCollectibleSystem::spawnPowerUps(int amount) {
	for (int i = 0; i < amount; i++) {
		spawnPowerUp(glm::vec3(rand() % 5, 1, rand() % 5), rand() % PowerUps::NUMPOWUPS - 1, 15, 30); // TODO: CHANGE TO READ FROM SETTINGS
	}
}
#ifdef DEVELOPMENT
unsigned int PowerUpCollectibleSystem::getByteSize() const {
	/* TODO: Fix component size */
	return sizeof(*this);
}
void PowerUpCollectibleSystem::imguiPrint(Entity** selectedEntity) {
	if (m_playerList) {
		ImGui::Text("PLAYERLISTI");
	}
	else {
		ImGui::Text("NO PLAYERLISTI");
	}

	for (float& d : m_distances) {
		ImGui::Text(std::to_string(d).c_str());
	}

	ImGui::Separator();
	
	for (auto& respawn : m_respawns) {
		ImGui::Text(std::to_string(respawn.time).c_str());
	}

	ImGui::Separator();
}

void PowerUpCollectibleSystem::spawnPowerUp(glm::vec3 pos, int powerUp, float time, float respawntime, Netcode::ComponentID compID) {
	Entity::SPtr e = EntityFactory::CreatePowerUp(pos, powerUp, compID);
	auto* pC = e->getComponent<PowerUpCollectibleComponent>();
	pC->respawnTime = respawntime;
	pC->powerUpDuration = time;

	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
			Netcode::MessageType::SPAWN_POWER_UP,
			SAIL_NEW Netcode::MessageSpawnPowerUp{
				powerUp,
				pos,
				e->getComponent<NetworkSenderComponent>()->m_id,
			}	
			, false
		);
	}
}
void PowerUpCollectibleSystem::updateSpawns(const float dt) {

	auto it = m_respawns.begin();
	while (it != m_respawns.end()) {
		it->time -= dt;
		if (it->time <= 0.0f) {
			spawnPowerUp(it->pos, it->powerUp, 15.0f, 30.0f); //TODO: change values to use settings
			it = m_respawns.erase(it);
		}
		else {
			it++;
		}
	}
}

void PowerUpCollectibleSystem::onDestroyPowerUp(const DestroyPowerUp& e) {
	for (auto pow : entities) {
		NetworkReceiverComponent* nrc = pow->getComponent<NetworkReceiverComponent>();
		PowerUpCollectibleComponent* powerCC = pow->getComponent<PowerUpCollectibleComponent>();

		if (nrc && nrc->m_id == e._netCompID) {
			//NetworkReceiverComponent* player_nrc;
			//for (auto& player : *m_playerList) {
			//	player_nrc = player->getComponent<NetworkReceiverComponent>();
			//	if (player_nrc && player_nrc->m_id && e.pickedByPlayer) {
			//		Netcode::PlayerID ownerId = Netcode::getComponentOwner(player_nrc->m_id);
			//		PowerUpComponent* playerpowerC = player->getComponent<PowerUpComponent>();

			//		playerpowerC->powerUps[powerCC->powerUp].addTime(powerCC->powerUpDuration);

			//	}
			//}

			pow->queueDestruction();
			break;
		}
	}
}

bool PowerUpCollectibleSystem::onEvent(const Event& e) {
	
	switch (e.type) {
	case Event::Type::SPAWN_POWERUP:
	{
		SpawnPowerUp& p = (SpawnPowerUp&)e;
		spawnPowerUp(p.pos, p.powerUpType, 0, 0, p._netCompID);
	}
		break;
	case Event::Type::DESTROY_POWERUP: onDestroyPowerUp((const DestroyPowerUp&)e);
	break;
	default:
		break;
	}
	
	return false;
}
#endif