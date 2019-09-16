#include "pch.h"
#include "DX12ForwardRenderer.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/Application.h"
#include "../DX12Utils.h"
#include "../shader/DX12ShaderPipeline.h"
#include "../resources/DescriptorHeap.h"
#include "Sail/graphics/shader/compute/TestComputeShader.h"
#include "Sail/api/ComputeShaderDispatcher.h"
#include "../resources/DX12Texture.h"
#include "Sail/graphics/postprocessing/PostProcessPipeline.h"
#include "API/DX12/resources/DX12RenderableTexture.h"

DX12ForwardRenderer::DX12ForwardRenderer() {
	Application* app = Application::getInstance();
	m_context = app->getAPI<DX12API>();
	m_context->initCommand(m_command);
	m_command.list->SetName(L"Forward Renderer main command list");

	//m_computeShaderDispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());

	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();
	m_outputTexture = std::unique_ptr<DX12RenderableTexture>(static_cast<DX12RenderableTexture*>(RenderableTexture::Create(windowWidth, windowHeight)));
}

DX12ForwardRenderer::~DX12ForwardRenderer() {

}

void DX12ForwardRenderer::present(PostProcessPipeline* postProcessPipeline, RenderableTexture* output) {
	assert(!output); // Render to texture is currently not implemented for DX12!

	auto frameIndex = m_context->getFrameIndex();

	auto& allocator = m_command.allocators[frameIndex];
	auto& cmdList = m_command.list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);
	
	// Transition output texture to render target
	//m_context->prepareToRender(cmdList.Get());
	//m_context->renderToBackBuffer(cmdList.Get());

	m_outputTexture->begin(cmdList.Get());
	m_outputTexture->clear({ 0.f,0.f,0.f,1.f }, cmdList.Get());

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// Compute shader testing
	//auto* computeShader = &Application::getInstance()->getResourceManager().getShaderSet<TestComputeShader>();
	//auto* mat = commandQueue.at(1).mesh->getMaterial();
	//TestComputeShader::Input computeInput;
	//computeInput.outputWidth = 400;
	//computeInput.outputHeight = 400;
	//computeInput.threadGroupCountX = computeInput.outputWidth;
	//computeInput.threadGroupCountY = computeInput.outputHeight;
	//computeInput.inputTexture = mat->getTexture(0);
	//m_computeShaderDispatcher->begin(cmdList.Get());
	//auto& computeOutput = m_computeShaderDispatcher->dispatch(*computeShader, computeInput, cmdList.Get());



	// Bind the descriptor heap that will contain all SRVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());

	// Bind mesh-common constant buffers (camera)
	// TODO: bind camera cbuffer here
	//cmdList->SetGraphicsRootConstantBufferView(GlobalRootParam::CBV_CAMERA, asdf);

	// TODO: Sort meshes according to material
	unsigned int meshIndex = 0;
	for (RenderCommand& command : commandQueue) {
		DX12ShaderPipeline* shaderPipeline = static_cast<DX12ShaderPipeline*>(command.mesh->getMaterial()->getShader()->getPipeline());

		// Set mesh index which is used to bind the correct cbuffers from the resource heap
		// The index order does not matter, as long as the same index is used for bind and setCBuffer
		shaderPipeline->setResourceHeapMeshIndex(meshIndex);

		shaderPipeline->bind(cmdList.Get());

		shaderPipeline->setCBufferVar("sys_mWorld", &glm::transpose(command.transform), sizeof(glm::mat4));
		shaderPipeline->setCBufferVar("sys_mVP", &camera->getViewProjection(), sizeof(glm::mat4));
		shaderPipeline->setCBufferVar("sys_cameraPos", &camera->getPosition(), sizeof(glm::vec3));

		if (lightSetup) {
			auto& dlData = lightSetup->getDirLightData();
			auto& plData = lightSetup->getPointLightsData();
			shaderPipeline->setCBufferVar("dirLight", &dlData, sizeof(dlData));
			shaderPipeline->setCBufferVar("pointLights", &plData, sizeof(plData));
		}

		command.mesh->draw(*this, cmdList.Get());
		meshIndex++;
	}

	RenderableTexture* renderOutput = m_outputTexture.get();
	// Run post processing
	if (postProcessPipeline) {
		renderOutput = postProcessPipeline->run(m_outputTexture.get(), cmdList.Get());
	}
	DX12RenderableTexture* dxRenderOutput = static_cast<DX12RenderableTexture*>(renderOutput);

	// Copy post processing output to back buffer
	dxRenderOutput->transitionStateTo(cmdList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	auto* renderTarget = m_context->getCurrentRenderTargetResource();
	DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->CopyResource(renderTarget, dxRenderOutput->getResource());
	// Lastly - transition back buffer to present
	DX12Utils::SetResourceTransitionBarrier(cmdList.Get(), renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	//m_context->prepareToPresent(cmdList.Get());
	// Execute command list
	cmdList->Close();
	m_context->executeCommandLists({ cmdList.Get() });

}
