#pragma once
#include "Component.h"
#include "Sail/netcode/NetworkedStructs.h"
#include <vector>


class NetworkSenderComponent : public Component<NetworkSenderComponent> {
public:
	/*
	  type will decide which components are modified for the entity this component belongs to
	  The ownerID is the ID of the player and is used to create a unique ID for the network component
	
	  The first byte of the object ID is set to the ownerID and the rest to the unique ID generated by the player
	*/
	NetworkSenderComponent(Netcode::EntityType entityType, Netcode::PlayerID ownerID) :
		m_id(Netcode::generateUniqueComponentID(ownerID)), m_entityType(entityType)
	{}
	NetworkSenderComponent(Netcode::EntityType entityType, Netcode::PlayerID ownerID, Netcode::MessageType msgType) :
		m_id(Netcode::generateUniqueComponentID(ownerID)), m_entityType(entityType), m_dataTypes{msgType} 
	{}

	/*
	  This should only be used if you've generated a unique component ID previously but haven't created the entity yet.
	  Currently this is only used for the creation of projectiles since their component ID are sent over the network
	  before they are created by the local player.
	*/
	NetworkSenderComponent(Netcode::EntityType entityType, Netcode::ComponentID objectID) :
		m_id(objectID), m_entityType(entityType)
	{}

	~NetworkSenderComponent() {}

	// Removes all message types so that this entity will no longer send any information
	void removeAllMessageTypes() {
		m_dataTypes.clear();
	}

	void addMessageType(Netcode::MessageType type) { 
		m_dataTypes.push_back(type); 
	}

	void removeMessageType(Netcode::MessageType type) {
		// Search through dataType vector
		for (int i = 0; i < m_dataTypes.size(); i++) {
			// If found...
			if (m_dataTypes.at(i) == type) {
				// ... Then delete
				m_dataTypes.erase(m_dataTypes.begin() + i);
			}
		}

		// Then, if size is 0, queue a deletion of this component.
		
	}

	bool messageTypeAlreadyExists(Netcode::MessageType type) {
		for (auto& t : m_dataTypes)	{
			if (t == type) {
				return true;
			}
		}
		return false;
	}

	Netcode::ComponentID m_id;
	Netcode::EntityType m_entityType;

	std::vector<Netcode::MessageType> m_dataTypes;

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this) + sizeof(Netcode::MessageType) * m_dataTypes.size();
	}
	void imguiRender(Entity** selected) {
		ImGui::Text(std::string("m_id: " + std::to_string(m_id)).c_str());
		ImGui::Text(std::string("m_entityType: " + (m_entityType == Netcode::EntityType::PLAYER_ENTITY) ? "PLAYER_ENTITY" : "MECHA_ENTITY").c_str());
	}
#endif
};
