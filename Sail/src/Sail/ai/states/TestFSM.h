#pragma once

#include "FiniteStateMachine.h"

class TestFSM : public FiniteStateMachine {
public:
	TestFSM(const std::string& name)
		: FiniteStateMachine(name)
	{}

	~TestFSM() {}

	void update(float dt, Entity* entity) override {
		Logger::Log("This is the " + m_name + "!");

		FiniteStateMachine::update(dt);
	}

private:

};