#pragma once
#include "Component.h"
class Model;

class ModelComponent : public Component<ModelComponent> {
public:
	ModelComponent(Model* model = nullptr)
		: m_model(model),
		renderToGBuffer(true)
	{ }
	~ModelComponent() { }

	void setModel(Model* model) {
		m_model = model;
	}

	Model* getModel() const {
		return m_model;
	}

	bool renderToGBuffer;
#ifdef DEVELOPMENT
	void imguiRender() {
		ImGui::Columns(2);
		ImGui::Checkbox("##VEL", &renderToGBuffer); ImGui::NextColumn();
		ImGui::Text(std::string("renderToGBuffer").c_str());
		ImGui::Columns(1);
	}
#endif

private:
	Model* m_model;
};