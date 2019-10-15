#pragma once

#include "Sail.h"
#include "Sail/entities/systems/SystemDeclarations.h"

class PerformanceTestState : public State {
public:
	PerformanceTestState(StateStack& stack);
	~PerformanceTestState();

	// Process input for the state ||
	virtual bool processInput(float dt) override;
	// Sends events to the state
	virtual bool onEvent(Event& event) override;
	// Updates the state - Runs every frame
	virtual bool update(float dt, float alpha = 1.0f) override;
	// Updates the state - Runs every tick
	virtual bool fixedUpdate(float dt) override;
	// Renders the state
	virtual bool render(float dt, float alpha = 1.0f) override;
	// Renders imgui
	virtual bool renderImgui(float dt) override;
	// If the state is about to change clean it up
	virtual bool prepareStateChange() override;


private:
	bool onResize(WindowResizeEvent& event);
	bool renderImGuiGameValues(float dt);
	void shutDownPerformanceTestState();

	// Where to updates the component systems. Responsibility can be moved to other places
	void updatePerTickComponentSystems(float dt);
	void updatePerFrameComponentSystems(float dt, float alpha);
	void runSystem(float dt, BaseComponentSystem* toRun);

	Entity::SPtr createCandleEntity(const std::string& name, Model* lightModel, Model* bbModel, glm::vec3 lightPos);

	void loadAnimations();
	void initAnimations();

	void populateScene(Model* characterModel, Model* lightModel, Model* bbModel, Model* projectileModel, Shader* shader);

private:
	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;
	FlyingCameraController m_camController;

	void createBots(Model* boundingBoxModel, Model* characterModel, Model* projectileModel, Model* lightModel);
	void createLevel(Shader* shader, Model* boundingBoxModel);

	Systems m_componentSystems;
	LightSetup m_lights;
	Profiler m_profiler;
	RenderSettingsWindow m_renderSettingsWindow;
	LightDebugWindow m_lightDebugWindow;

	std::vector<Entity::SPtr> m_performanceEntities;

	size_t m_currLightIndex;

	bool m_paused = false;
	bool m_isSingleplayer = true;

	Octree* m_octree;
	bool m_showcaseProcGen;

	std::bitset<MAX_NUM_COMPONENTS_TYPES> m_currentlyWritingMask;
	std::bitset<MAX_NUM_COMPONENTS_TYPES> m_currentlyReadingMask;

	std::vector<std::future<BaseComponentSystem*>> m_runningSystemJobs;
	std::vector<BaseComponentSystem*> m_runningSystems;

	bool m_poppedThisFrame = false;

};