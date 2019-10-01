#pragma once

#include "Sail/api/Renderer.h"
#include "../DX12API.h"

class DX12RenderableTexture;
class PostProcessPipeline;

#define MULTI_THREADED_COMMAND_RECORDING
//#define DEBUG_MULTI_THREADED_COMMAND_RECORDING

class DX12GBufferRenderer : public Renderer {
public:
	static const int NUM_GBUFFERS = 2;

public:
	DX12GBufferRenderer();
	~DX12GBufferRenderer();

	void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual bool onEvent(Event& event) override;

	DX12RenderableTexture** getGBufferOutputs() const;

private:
	bool onResize(WindowResizeEvent& event);

private:
	static const int MAX_RECORD_THREADS = 1;
	static const int MIN_COMMANDS_PER_THREAD = 20;


	DX12API* m_context;
	DX12RenderableTexture* m_gbufferTextures[NUM_GBUFFERS];
	DX12API::Command m_command[MAX_RECORD_THREADS];

	void recordCommands(PostProcessPipeline* postProcessPipeline, const int threadID, const int frameIndex, const int start, const int nCommands, size_t oobMax, int nThreads);
};