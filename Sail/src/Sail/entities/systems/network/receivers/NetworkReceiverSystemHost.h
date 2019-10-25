#pragma once


#include "NetworkReceiverSystem.h"



class NetworkReceiverSystemHost : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemHost();
	~NetworkReceiverSystemHost();


	// Push incoming data strings to the back of a FIFO list
	void handleIncomingData(std::string data) override;

	void endMatch() override;
	void endMatchAfterTimer() override;
	void prepareEndScreen(int bf, float dw, int jm, Netcode::PlayerID id);

private:
	bool m_startEndGameTimer = false;
};