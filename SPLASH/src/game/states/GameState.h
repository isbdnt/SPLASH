#pragma once

#include "Sail.h"

#include "Physics.h"

class GameState : public State {
public:
	GameState(StateStack& stack);
	~GameState();

	// Process input for the state
	virtual bool processInput(float dt) override;
	// Sends events to the state
	virtual bool onEvent(Event& event) override;
	// Updates the state
	virtual bool update(float dt) override;
	// Renders the state
	virtual bool render(float dt) override;
	// Renders imgui
	virtual bool renderImgui(float dt) override;

private:
	bool onResize(WindowResizeEvent& event);

private:
	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;
	FlyingCameraController m_camController;

	Entity::SPtr m_texturedCubeEntity;
	std::vector<Entity::SPtr> m_transformTestEntities;

	Scene m_scene;
	LightSetup m_lights;

	std::unique_ptr<Model> m_cubeModel;
	std::unique_ptr<Model> m_planeModel;

	std::unique_ptr<Model> m_boundingBoxModel;

	Octree* m_octree;

	std::vector<BoundingBox*> m_testBoundingBoxes;

	float m_testAngle;
};