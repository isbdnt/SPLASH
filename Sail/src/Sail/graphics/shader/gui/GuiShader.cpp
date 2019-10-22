#include "pch.h"
#include "GuiShader.h"


GuiShader::GuiShader()
	: Shader("gui/GuiShader.hlsl")
{
	shaderPipeline->getInputLayout().pushVec2(InputLayout::POSITION, "POSITION", 0);
	shaderPipeline->getInputLayout().pushVec2(InputLayout::TEXCOORD, "TEXCOORD", 0);
	shaderPipeline->getInputLayout().create(shaderPipeline->getVsBlob());


	// Finish the shader creation
	finish();
}

GuiShader::~GuiShader() {
}

void GuiShader::bind() {
	Shader::bind();
}