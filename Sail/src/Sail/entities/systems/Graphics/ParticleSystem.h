#pragma once
#include "..//BaseComponentSystem.h"
#include "../../components/ParticleEmitterComponent.h"

struct ID3D12GraphicsCommandList4;

class ParticleSystem final : public BaseComponentSystem {
public:
	ParticleSystem();
	~ParticleSystem();

	void update(float dt);
	void updateOnGPU(ID3D12GraphicsCommandList4* cmdList, const glm::vec3& cameraPos);

	void submitAll() const;

private:
	void initEmitter(Entity* owner, ParticleEmitterComponent* component);
	virtual void stop() override;

private:
	std::unique_ptr<ComputeShaderDispatcher> m_dispatcher;

	std::unordered_map<Entity*, ParticleEmitterComponent::EmitterData> m_emitters;

};