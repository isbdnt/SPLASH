#include "pch.h"
#include "ParticleSystem.h"
#include "Sail/Application.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/Entity.h"
#include "API/DX12/DX12VertexBuffer.h"
#include "API/DX12/shader/DX12ConstantBuffer.h"
#include "API/DX12/shader/DX12StructuredBuffer.h"
#include "API/DX12/resources/DescriptorHeap.h"
#include "API/DX12/DX12Utils.h"
#include "API/DX12/DX12API.h"
#include "Sail/utils/Timer.h"

#include "Sail/graphics/shader/compute/ParticleComputeShader.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"

ParticleSystem::ParticleSystem() {
	registerComponent<ParticleEmitterComponent>(true, true, true);

	m_particleShader = &Application::getInstance()->getResourceManager().getShaderSet<ParticleComputeShader>();
	m_dispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());
	auto& gbufferShader = Application::getInstance()->getResourceManager().getShaderSet<GBufferOutShader>();
	auto& inputLayout = gbufferShader.getPipeline()->getInputLayout();

	//m_outputVertexBufferSize = 9996;
	m_outputVertexBufferSize = 36;

	m_model = std::make_unique<Model>(m_outputVertexBufferSize, &gbufferShader);

	m_outputVertexBuffer = static_cast<DX12VertexBuffer*>(&m_model->getMesh(0)->getVertexBuffer());
	
	void* initData = malloc(m_outputVertexBufferSize * 4);
	memset(initData, 0, m_outputVertexBufferSize * 4);
	m_physicsBuffer = std::make_unique<ShaderComponent::DX12StructuredBuffer>(initData, m_outputVertexBufferSize / 6, 6*4);
	free(initData);

	m_numberOfParticles = 0;

	m_cpuOutput = SAIL_NEW CPUOutput[DX12API::NUM_GPU_BUFFERS];
	for (unsigned int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		m_cpuOutput[i].previousNrOfParticles = 0;
		m_cpuOutput[i].lastFrameTime = 0;
	}

	m_timer.startTimer();
	m_startTime = m_timer.getStartTime();
	m_gpuUpdates = 0;
}

ParticleSystem::~ParticleSystem() {
	delete[] m_cpuOutput;
}

void ParticleSystem::spawnParticles(int particlesToSpawn, ParticleEmitterComponent* particleEmitterComp) {
	//Spawn particles for all swap buffers
	for (unsigned int i = 0; i < DX12API::NUM_GPU_BUFFERS; i++) {
		m_cpuOutput[i].newEmitters.emplace_back();
		m_cpuOutput[i].newEmitters.back().nrOfNewParticles = particlesToSpawn;
		m_cpuOutput[i].newEmitters.back().emitter = particleEmitterComp;
	}

	m_numberOfParticles += particlesToSpawn;
}

void ParticleSystem::update(float dt) {
	for (auto& e : entities) {
		ParticleEmitterComponent* particleEmitterComp = e->getComponent<ParticleEmitterComponent>();

		if (particleEmitterComp->spawnTimer >= particleEmitterComp->spawnRate) {
			//Spawn the correct number of particles
			int particlesToSpawn = (int)glm::floor(particleEmitterComp->spawnTimer / glm::max(particleEmitterComp->spawnRate, 0.0001f));
			spawnParticles(particlesToSpawn, particleEmitterComp);
			//Decrease timer
			particleEmitterComp->spawnTimer -= particleEmitterComp->spawnRate * particlesToSpawn;
		}
		particleEmitterComp->spawnTimer += dt;
	}
}

void ParticleSystem::updateOnGPU(ID3D12GraphicsCommandList4* cmdList) {
	auto* context = Application::getInstance()->getAPI<DX12API>();

	if (m_gpuUpdates < DX12API::NUM_GPU_BUFFERS * 2) {
		//Initialize timers the first two times the buffers are ran
		float elapsedTime = m_timer.getTimeSince<float>(m_startTime) - m_cpuOutput[context->getSwapIndex()].lastFrameTime;
		m_cpuOutput[context->getSwapIndex()].lastFrameTime += elapsedTime;
		m_gpuUpdates++;
	}
	else {
		m_outputVertexBuffer->init(cmdList);

		m_dispatcher->begin(cmdList);

		auto* settings = m_particleShader->getComputeSettings();

		//----Compute shader constant buffer----
		ComputeInput inputData;
		inputData.numEmitters = m_cpuOutput[context->getSwapIndex()].newEmitters.size();
		inputData.previousNrOfParticles = m_cpuOutput[context->getSwapIndex()].previousNrOfParticles;
		inputData.maxOutputVertices = m_outputVertexBufferSize;
		float elapsedTime = m_timer.getTimeSince<float>(m_startTime) - m_cpuOutput[context->getSwapIndex()].lastFrameTime;
		inputData.frameTime = elapsedTime;

		//Update timer for this buffer
		m_cpuOutput[context->getSwapIndex()].lastFrameTime += elapsedTime;

		for (unsigned int i = 0; i < m_cpuOutput[context->getSwapIndex()].newEmitters.size(); i++) {
			inputData.emitters[i].position = m_cpuOutput[context->getSwapIndex()].newEmitters[i].emitter->position;
			inputData.emitters[i].velocity = m_cpuOutput[context->getSwapIndex()].newEmitters[i].emitter->velocity;
			inputData.emitters[i].acceleration = m_cpuOutput[context->getSwapIndex()].newEmitters[i].emitter->acceleration;
			inputData.emitters[i].nrOfParticlesToSpawn = m_cpuOutput[context->getSwapIndex()].newEmitters[i].nrOfNewParticles;
		}

		m_particleShader->getPipeline()->setCBufferVar("inputBuffer", &inputData, sizeof(ComputeInput));
		//--------------------------------------

		auto& cdh = context->getComputeGPUDescriptorHeap()->getCurentCPUDescriptorHandle();
		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize() * 10;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_outputVertexBufferSize;
		uavDesc.Buffer.StructureByteStride = 4 * 14; // TODO: replace with sizeof(Vertex)
		context->getDevice()->CreateUnorderedAccessView(m_outputVertexBuffer->getBuffer(), nullptr, &uavDesc, cdh);
		// Transition to UAV access
		DX12Utils::SetResourceTransitionBarrier(cmdList, m_outputVertexBuffer->getBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//----Compute shader physics buffer----
		cdh.ptr += context->getComputeGPUDescriptorHeap()->getDescriptorIncrementSize();
		uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_outputVertexBufferSize / 6;
		uavDesc.Buffer.StructureByteStride = 6*4; // TODO: replace with sizeof(ParticlePhysics)
		context->getDevice()->CreateUnorderedAccessView(m_physicsBuffer->getBuffer(), nullptr, &uavDesc, cdh);
		// Transition to UAV access
		if (m_gpuUpdates < DX12API::NUM_GPU_BUFFERS * 2 + 2) {
			DX12Utils::SetResourceTransitionBarrier(cmdList, m_physicsBuffer->getBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			m_gpuUpdates++;
		}
		//-------------------------------------

		m_dispatcher->dispatch(*m_particleShader, Shader::ComputeShaderInput(), 0, cmdList);

		context->getComputeGPUDescriptorHeap()->getAndStepIndex(11);

		// Transition to Cbuffer usage
		DX12Utils::SetResourceTransitionBarrier(cmdList, m_outputVertexBuffer->getBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// Update nr of particles in this buffer and clear the newEmitters list
		m_cpuOutput[context->getSwapIndex()].previousNrOfParticles = glm::min(m_numberOfParticles, (int)(m_outputVertexBufferSize / 6));
		m_cpuOutput[context->getSwapIndex()].newEmitters.clear();
	}
}

void ParticleSystem::submitAll() const {
	Renderer* renderer = Application::getInstance()->getRenderWrapper()->getCurrentRenderer();
	Renderer::RenderFlag flags = Renderer::MESH_DYNAMIC;
	flags |= Renderer::IS_VISIBLE_ON_SCREEN;

	renderer->submit(
		m_model.get(),
		glm::identity<glm::mat4>(),
		flags
	);

}
