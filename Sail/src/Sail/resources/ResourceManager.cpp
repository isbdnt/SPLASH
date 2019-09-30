#include "pch.h"
#include "ResourceManager.h"
//#include "../graphics/shader/deferred/DeferredGeometryShader.h"
//#include "audio/SoundManager.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/api/shader/ShaderPipeline.h"

const std::string ResourceManager::SAIL_DEFAULT_MODEL_LOCATION = "res/models/";

ResourceManager::ResourceManager() {
	//m_soundManager = std::make_unique<SoundManager>();
	m_assimpLoader = std::make_unique<AssimpLoader>();
	m_fbxLoader = std::make_unique<FBXLoader>();
}
ResourceManager::~ResourceManager() {
	for (auto it : m_shaderSets) {
		delete it.second;
	}
}

//
// AudioData
//

void ResourceManager::loadAudioData(const std::string& filename, IXAudio2* xAudio2) {
	if (!this->hasAudioData(filename)) {
		m_audioDataAll.insert({ filename, std::make_unique<AudioData>(filename, xAudio2) });
	} 
}

AudioData& ResourceManager::getAudioData(const std::string& filename) {
	auto pos = m_audioDataAll.find(filename);
	if (pos == m_audioDataAll.end()) {
		Logger::Error("Tried to access an audio resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadAudioData(\"filename\") before accessing it.");
	}

	return *pos->second;
}

bool ResourceManager::hasAudioData(const std::string& filename) {
	return m_audioDataAll.find(filename) != m_audioDataAll.end();
}

//
// TextureData
//

void ResourceManager::loadTextureData(const std::string& filename) {
	m_textureDatas.insert({ filename, std::make_unique<TextureData>(filename) });
}
TextureData& ResourceManager::getTextureData(const std::string& filename) {
	auto pos = m_textureDatas.find(filename);
	if (pos == m_textureDatas.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadTextureData(\"filename\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTextureData(const std::string& filename) {
	return m_textureDatas.find(filename) != m_textureDatas.end();
}

//
// DXTexture
//

void ResourceManager::loadTexture(const std::string& filename) {
	m_textures.insert({ filename, std::unique_ptr<Texture>(Texture::Create(filename)) });
}
Texture& ResourceManager::getTexture(const std::string& filename) {
	auto pos = m_textures.find(filename);
	if (pos == m_textures.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().loadTexture(\"" + filename + "\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTexture(const std::string& filename) {
	return m_textures.find(filename) != m_textures.end();
}


//
// Model
//

void ResourceManager::loadModel(const std::string& filename, Shader* shader, const ImporterType type) {
	// Insert the new model

	Model* temp = nullptr;
	if(type == ResourceManager::ImporterType::SAIL_ASSIMP)
		temp = m_assimpLoader->importModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shader);
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		temp = m_fbxLoader->fetchModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shader);
	}

	if (temp) {

		m_models.insert({ filename, std::unique_ptr<Model>(temp) });
	}
	else {
#ifdef _DEBUG
		Logger::Error("Could not Load model: (" + filename + ")");
		//assert(temp);
#endif
	}
}
Model& ResourceManager::getModel(const std::string& filename, Shader* shader) {
	auto pos = m_models.find(filename);
	if (pos == m_models.end()) {
		// Model was not yet loaded, load it and return
		loadModel(filename, shader);
		
		return *m_models.find(filename)->second;
		//Logger::Error("Tried to access an fbx model that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadFBXModel(" + filename + ") before accessing it.");
	}

	return *pos->second;
}
bool ResourceManager::hasModel(const std::string& filename) {
	return m_models.find(filename) != m_models.end();
}

void ResourceManager::loadAnimationStack(const std::string& fileName, const ImporterType type) {
	AnimationStack* temp = nullptr;
	if (type == ResourceManager::ImporterType::SAIL_ASSIMP)
		temp = m_assimpLoader->importAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName);
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		//TODO: REMOVE SHADER FROM ANIMATION IMPORT
		temp = m_fbxLoader->fetchAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName, nullptr);
	}

	if (temp) {

		m_animationStacks.insert({ fileName, std::unique_ptr<AnimationStack>(temp) });
	}
	else {
#ifdef _DEBUG
		Logger::Error("Could not Load model: (" + fileName + ")");
		//assert(temp);
#endif
	}


	m_animationStacks.insert({ fileName, std::unique_ptr<AnimationStack>(m_assimpLoader->importAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName))});
}

AnimationStack& ResourceManager::getAnimationStack(const std::string& fileName) {
	//TODO : make more reliable
	if (m_animationStacks.find(fileName) == m_animationStacks.end()) {
		loadAnimationStack(fileName);
	}
	return *m_animationStacks[fileName].get();
}

bool ResourceManager::hasAnimationStack(const std::string& fileName) {
	return false;
}

//void ResourceManager::reloadShaders() {
//	for (auto it = m_shaderSets.begin(); it != m_shaderSets.end(); ++it)
//		it->second->reload();
//}


// Sound Manager
//SoundManager* ResourceManager::getSoundManager() {
//	return m_soundManager.get();
//}