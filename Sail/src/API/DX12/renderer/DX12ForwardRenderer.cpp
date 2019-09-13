#include "pch.h"
#include "DX12ForwardRenderer.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/graphics/shader/Shader.h"
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

	for (size_t i = 0; i < nRecordThreads; i++)
	{
		m_context->initCommand(m_command[i]);
		std::wstring name = L"Forward Renderer main command list for render thread: " + std::to_wstring(i);
		m_command[i].list->SetName(name.c_str());
	}

}

DX12ForwardRenderer::~DX12ForwardRenderer() {

}

void DX12ForwardRenderer::present(RenderableTexture* output) {
	assert(!output); // Render to texture is currently not implemented for DX12!

	auto frameIndex = m_context->getFrameIndex();
	size_t count = commandQueue.size();

#ifdef MULTI_THREADED_COMMAND_RECORDING
	std::future<void> fut[nRecordThreads];
	int commandsPerThread = round(count / (float)nRecordThreads);
	int start = 0;
	for (size_t i = 0; i < nRecordThreads; i++)
	{
		fut[i] = Application::getInstance()->pushJobToThreadPool(
			[this, i, frameIndex, start, commandsPerThread, count](int id) {
				return this->RecordCommands(i, frameIndex, start, (i < nRecordThreads - 1) ? commandsPerThread : commandsPerThread + 1, count);
			});
		start += commandsPerThread;
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].get(); //Force recording threads to be recorded in order
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
	}

	ID3D12CommandList* commandlists[nRecordThreads];
 	for (size_t i = 0; i < nRecordThreads; i++)
	{
#ifndef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		fut[i].wait(); //Wait for all recording threads to finnish
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
		commandlists[i] = m_command[i].list.Get();
	}

	m_context->executeCommandLists(commandlists, nRecordThreads);
#else
	RecordCommands(0, frameIndex, 0, count, count);
	m_context->executeCommandLists({ m_command[0].list.Get() });
#endif // MULTI_THREADED_COMMAND_RECORDING
}

void DX12ForwardRenderer::RecordCommands(const int threadID, const int frameIndex, const int start, const int nCommands, size_t oobMax)
{
//#ifdef MULTI_THREADED_COMMAND_RECORDING
	auto& allocator = m_command[threadID].allocators[frameIndex];
	auto& cmdList = m_command[threadID].list;

	// Reset allocators and lists for this frame
	allocator->Reset();
	cmdList->Reset(allocator.Get(), nullptr);

	// Transition back buffer to render target

	m_context->renderToBackBuffer(cmdList.Get());

#ifdef MULTI_THREADED_COMMAND_RECORDING
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
	if (threadID == 0) {
		m_context->prepareToRender(cmdList.Get());
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Prep to render, and record. " + std::to_string(start) + " to " + std::to_string(start+nCommands));
	}
	else if(threadID < nRecordThreads - 1){
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Recording Only. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
	}
#else
	if (threadID == 0) {
		m_context->prepareToRender(cmdList.Get());
	}
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
#else
	m_context->prepareToRender(cmdList.Get());
#endif

	cmdList->SetGraphicsRootSignature(m_context->getGlobalRootSignature());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the descriptor heap that will contain all SRVs for this frame
	m_context->getMainGPUDescriptorHeap()->bind(cmdList.Get());

	// Bind mesh-common constant buffers (camera)
	// TODO: bind camera cbuffer here
	//cmdList->SetGraphicsRootConstantBufferView(GlobalRootParam::CBV_CAMERA, asdf);

	// TODO: Sort meshes according to material
	unsigned int meshIndex = start;
	RenderCommand* command;
	for (int i = 0; i < nCommands && meshIndex < oobMax; i++, meshIndex++ /*RenderCommand& command : commandQueue*/) {
		command = &commandQueue[meshIndex];
		DX12ShaderPipeline* shaderPipeline = static_cast<DX12ShaderPipeline*>(command->mesh->getMaterial()->getShader()->getPipeline());

		// Set mesh index which is used to bind the correct cbuffers from the resource heap
		// The index order does not matter, as long as the same index is used for bind and setCBuffer
		shaderPipeline->setResourceHeapMeshIndex(meshIndex);

		shaderPipeline->bind(cmdList.Get());

		shaderPipeline->setCBufferVar("sys_mWorld", &glm::transpose(command->transform), sizeof(glm::mat4));
		shaderPipeline->setCBufferVar("sys_mVP", &camera->getViewProjection(), sizeof(glm::mat4));
		shaderPipeline->setCBufferVar("sys_cameraPos", &camera->getPosition(), sizeof(glm::vec3));

		if (lightSetup) {
			auto& dlData = lightSetup->getDirLightData();
			auto& plData = lightSetup->getPointLightsData();
			shaderPipeline->setCBufferVar("dirLight", &dlData, sizeof(dlData));
			shaderPipeline->setCBufferVar("pointLights", &plData, sizeof(plData));
		}

		command->mesh->draw(*this, cmdList.Get());
		//meshIndex++;
	}

	// Lastly - transition back buffer to present
#ifdef MULTI_THREADED_COMMAND_RECORDING
	if (threadID == nRecordThreads - 1) {
		m_context->prepareToPresent(cmdList.Get());
#ifdef DEBUG_MULTI_THREADED_COMMAND_RECORDING
		Logger::Log("ThreadID: " + std::to_string(threadID) + " - Record and prep to present. " + std::to_string(start) + " to " + std::to_string(start + nCommands));
#endif // DEBUG_MULTI_THREADED_COMMAND_RECORDING
	}
#else
	m_context->prepareToPresent(cmdList.Get());
#endif
	// Execute command list
	cmdList->Close();
}