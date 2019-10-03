#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/ai/pathfinding/NodeSystem.h"

class TransformComponent;
class PhysicsComponent;
class AiComponent;
class GunComponent;
class NodeSystem;
class Model;
class Octree;

#ifdef _DEBUG_NODESYSTEM
class Shader;
#endif

class AiSystem final : public BaseComponentSystem {
public:
	AiSystem();
	~AiSystem();

	void initNodeSystem(Model* bbModel, Octree* octree);
#ifdef _DEBUG_NODESYSTEM
	void initNodeSystem(Model* bbModel, Octree* octree, Shader* shader);
#endif

	/*
		Adds an entity to the system
	*/
	bool addEntity(Entity* entity) override;

	std::vector<Entity*>& getEntities();

	void update(float dt) override;

	NodeSystem* getNodeSystem();

private:
	struct AiEntity {
		TransformComponent* transComp;
		PhysicsComponent* physComp;
		AiComponent* aiComp;
	};

	void updatePath(AiComponent* aiComp, TransformComponent* transComp);
	void updatePhysics(AiComponent* aiComp, TransformComponent* transComp, PhysicsComponent* physComp, float dt);
	float getAiYaw(PhysicsComponent* physComp, float currYaw, float dt);
	void aiUpdateFunc(Entity* entity, const float dt);
	glm::vec3& getDesiredDir(AiComponent* aiComp, TransformComponent* transComp);

private:
	std::unordered_map<int, AiEntity> m_aiEntities;

	float m_timeBetweenPathUpdate;

	std::unique_ptr<NodeSystem> m_nodeSystem;

	Octree* m_octree;

};