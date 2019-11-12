#pragma once

#include "Network/NetworkStructs.hpp"
#include "../../SPLASH/src/game/states/LobbyState.h"	// Needed for 'Message'
#include "Sail.h"
#include "Sail/netcode/NetcodeTypes.h"

#include <string>

#include "Sail/states/StateIdentifiers.h"


#define MAX_NAME_LENGTH 100


class Network;

// Move elsewhere?
struct Player {
	Netcode::PlayerID id;
	std::string name;
	bool justJoined = true;

	Player(Netcode::PlayerID setID = HOST_ID, std::string setName = "Hans")
		: name(setName), id(setID)
	{
		name.reserve(MAX_NAME_LENGTH);
	}

	bool friend operator==(const Player& left, const Player& right) {
		if (left.id == right.id &&
			left.name == right.name) {
			return true;
		}
		return false;
	}
};


class NWrapper : public NetworkEventHandler {
public:
	NWrapper(Network* pNetwork);
	virtual ~NWrapper();

	virtual bool host(int port = 54000) = 0;
	virtual bool connectToIP(char* = "127.0.0.1:54000") = 0;

	void sendMsg(std::string msg, TCP_CONNECTION_ID tcp_id = 0);
	void sendMsgAllClients(std::string msg);		// by either client or host
	void sendChatAllClients(std::string msg);		//
	virtual void sendChatMsg(std::string msg) = 0;
	void sendSerializedDataAllClients(std::string data);
	void sendSerializedDataToHost(std::string data);

	/*
		Host Only
			
		This will request a clients to enter a new state. GameState, EndGameState etc.
		playerId == 255 will send to all
	*/
	virtual void setClientState(States::ID state, Netcode::PlayerID playerId = 255) = 0;

protected:
	Network* m_network = nullptr;
	Application* m_app = nullptr;	

	// Parsing functions | Will alter 'data' upon being used.
	TCP_CONNECTION_ID parseID(std::string& data);	//
	std::string parseName(std::string& data);		//
	Message processChatMessage(const char* data);

private:
	friend class NWrapperSingleton;
	void initialize(Network* pNetwork);
	void handleNetworkEvents(NetworkEvent nEvent);

	virtual void playerJoined(TCP_CONNECTION_ID id) = 0;
	virtual void playerDisconnected(TCP_CONNECTION_ID id) = 0;
	virtual void playerReconnected(TCP_CONNECTION_ID id) = 0;
	virtual void decodeMessage(NetworkEvent nEvent) = 0;
};