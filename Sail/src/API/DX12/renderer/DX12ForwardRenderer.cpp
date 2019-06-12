#include "pch.h"
#include "DX12ForwardRenderer.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "../shader/DX12ShaderPipeline.h"
#include "../resources/DescriptorHeap.h"

Renderer* Renderer::Create(Renderer::Type type) {
	switch (type) {
	case FORWARD:
		return new DX12ForwardRenderer();
	default:
		Logger::Error("Tried to create a renderer of unknown or unimplemented type: " + std::to_string(type));
	}
	return nullptr;
}

DX12ForwardRenderer::DX12ForwardRenderer() {
	m_context = Application::getInstance()->getAPI<DX12API>();
	m_context->initCommand(m_command);
	m_command.list->SetName(L"Forward Renderer main command list");
}

DX12ForwardRenderer::~DX12ForwardRenderer() {

}

void DX12ForwardRenderer::present(RenderableTexture* output) {
	auto frameIndex = m_context->getFrameIndex();

	auto& allocator = m_command.allocators[frameIndex];
	auto& cmdList = m_command.list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	m_context->renderToBackBuffer(cmdList.Get());
	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the descriptor heap that will contain all SRVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());
	cmdList->SetGraphicsRootDescriptorTable(m_context->getRootIndexFromRegister("t0"), m_context->getMainGPUDescriptorHeap()->getGPUDescriptorHandleForIndex(0));

	// Bind mesh-common constant buffers (camera)
	// TODO: bind camera cbuffer here
	//cmdList->SetGraphicsRootConstantBufferView(GlobalRootParam::CBV_CAMERA, asdf);


	unsigned int meshIndex = 0;
	for (RenderCommand& command : commandQueue) {
		DX12ShaderPipeline* shader = static_cast<DX12ShaderPipeline*>(command.mesh->getMaterial()->getShader());

		// Set mesh index which is used to bind the correct cbuffers from the resource heap
		// The index order does not matter, as long as the same index is used for bind and setCBuffer
		shader->setResourceHeapMeshIndex(meshIndex);

		shader->bind(cmdList.Get());

		shader->setCBufferVar("sys_mWorld", &glm::transpose(command.transform), sizeof(glm::mat4));
		shader->setCBufferVar("sys_mVP", &camera->getViewProjection(), sizeof(glm::mat4));
		shader->setCBufferVar("sys_cameraPos", &camera->getPosition(), sizeof(glm::vec3));

		if (lightSetup) {
			auto& dlData = lightSetup->getDirLightData();
			auto& plData = lightSetup->getPointLightsData();
			shader->setCBufferVar("dirLight", &dlData, sizeof(dlData));
			shader->setCBufferVar("pointLights", &plData, sizeof(plData));
		}

		command.mesh->draw(*this, cmdList.Get());
		meshIndex++;
	}

	// Lastly - transition back buffer to present
	m_context->prepareToPresent(cmdList.Get());
	// Execute command list
	cmdList->Close();
	m_context->executeCommandLists({ cmdList.Get() });

}
