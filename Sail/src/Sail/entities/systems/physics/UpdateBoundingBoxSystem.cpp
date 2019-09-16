#include "pch.h"
#include "UpdateBoundingBoxSystem.h"
#include "../../Entity.h"
#include "../../components/TransformComponent.h"
#include "../../components/BoundingBoxComponent.h"
#include "../../components/ModelComponent.h"

#include "../../../graphics/geometry/Model.h"

UpdateBoundingBoxSystem::UpdateBoundingBoxSystem() : BaseComponentSystem() {
	m_requiredComponentTypes.push_back(BoundingBoxComponent::ID);
}

UpdateBoundingBoxSystem::~UpdateBoundingBoxSystem() {
}

void UpdateBoundingBoxSystem::checkDistances(glm::vec3& minVec, glm::vec3& maxVec, const glm::vec3& testVec) {
	if (testVec.x > maxVec.x) {
		maxVec.x = testVec.x;
	}
	if (testVec.x < minVec.x) {
		minVec.x = testVec.x;
	}
	if (testVec.y > maxVec.y) {
		maxVec.y = testVec.y;
	}
	if (testVec.y < minVec.y) {
		minVec.y = testVec.y;
	}
	if (testVec.z > maxVec.z) {
		maxVec.z = testVec.z;
	}
	if (testVec.z < minVec.z) {
		minVec.z = testVec.z;
	}
}

void UpdateBoundingBoxSystem::update(float dt) {
	for (auto& e : m_entities) {
		BoundingBoxComponent* boundingBox = e->getComponent<BoundingBoxComponent>();

		TransformComponent* transform = e->getComponent<TransformComponent>();
		ModelComponent* model = e->getComponent<ModelComponent>();

		if (transform && model) {
			if (transform->getChange()) {
				glm::vec3 minPositions(INFINITY), maxPositions(-INFINITY);

				for (unsigned int i = 0; i < model->getModel()->getNumberOfMeshes(); i++) {
					const Mesh::Data& meshData = model->getModel()->getMesh(i)->getData();
					for (unsigned int j = 0; j < meshData.numVertices; j++) {
						glm::vec3 posAfterTransform = glm::vec3(transform->getMatrix() * glm::vec4(meshData.positions[j].vec, 1.0f));
						checkDistances(minPositions, maxPositions, posAfterTransform);
					}
				}

				boundingBox->getBoundingBox()->setHalfSize((maxPositions - minPositions) * 0.5f);
				boundingBox->getBoundingBox()->setPosition(minPositions + boundingBox->getBoundingBox()->getHalfSize());
			}
		}

		boundingBox->getTransform()->setTranslation(boundingBox->getBoundingBox()->getPosition());
		boundingBox->getTransform()->setScale(boundingBox->getBoundingBox()->getHalfSize() * 2.0f);
	}
}
