#include "pch.h"
#include "ResourceManager.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/api/Mesh.h"
#include "API/DX12/resources/DX12DDSTexture.h"

#include <filesystem>

// Horrible, I know
// But "needed" for filling a command list with finished textures
#include "API/DX12/resources/DX12Texture.h"

#include <iostream>
#include <fstream>

//#define USE_ONLY_THIS_TEXTURE "missing.tga"

const std::string ResourceManager::SAIL_DEFAULT_MODEL_LOCATION = "res/models/";
const std::string ResourceManager::SAIL_DEFAULT_SOUND_LOCATION = "res/sounds/";
const std::string ResourceManager::SAIL_DEFAULT_TEXTURE_LOCATION = "res/textures/";

ResourceManager::ResourceManager() {
	m_fbxLoader = std::make_unique<FBXLoader>();
	for (int i = 0; i < 5; i++) {
		m_byteSize[i] = 0;
	}
	m_byteSize[RMDataType::Generic] += sizeof(*this);
	m_defaultShader = nullptr;
}
ResourceManager::~ResourceManager() {
	for (auto it : m_shaderSets) {
		delete it.second;
	}
}

//
// AudioData
//

bool ResourceManager::setDefaultShader(Shader* shader) {
	if (shader) {
		m_defaultShader = shader;
		return true;
	}
	return false;
}

void ResourceManager::loadAudioData(const std::string& filename, IXAudio2* xAudio2) {	
	if (!this->hasAudioData(filename)) {
		auto inserted = m_audioDataAll.insert({ filename, std::make_unique<AudioData>(SAIL_DEFAULT_SOUND_LOCATION + filename, xAudio2) });
		if (inserted.second) {
			m_byteSize[RMDataType::Audio] += inserted.first->second->getByteSize();
		}
	} 
}

AudioData& ResourceManager::getAudioData(const std::string& filename) {
	auto pos = m_audioDataAll.find(filename);
	if (pos == m_audioDataAll.end()) {
		SAIL_LOG_ERROR("Tried to access an audio resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadAudioData(\"filename\") before accessing it.");
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
	SAIL_LOG_WARNING(filename + " should be swapped out for a dds version.");

	std::unique_lock<std::mutex> lock(m_textureDatasMutex);
	auto inserted = m_textureDatas.insert({ filename, std::make_unique<TextureData>(filename) });
	if (inserted.second) {
		m_byteSize[RMDataType::Textures] = calculateTextureByteSize();
	}
}
TextureData& ResourceManager::getTextureData(const std::string& filename) {
	std::unique_lock<std::mutex> lock(m_textureDatasMutex);
	auto pos = m_textureDatas.find(filename);
	if (pos == m_textureDatas.end())
		SAIL_LOG_ERROR("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadTextureData(\"filename\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTextureData(const std::string& filename) {
	std::unique_lock<std::mutex> lock(m_textureDatasMutex);
	return m_textureDatas.find(filename) != m_textureDatas.end();
}

//
// DXTexture
//

void ResourceManager::loadTexture(const std::string& filename) {
#ifdef USE_ONLY_THIS_TEXTURE
	*const_cast<std::string*>(&filename) = std::string(USE_ONLY_THIS_TEXTURE);
	if (hasTexture(filename)) {
		return;
	}
#endif

	auto path = std::filesystem::path(SAIL_DEFAULT_TEXTURE_LOCATION + filename);
	if (path.has_extension() && std::filesystem::exists(path)) {
		if (path.extension().compare(".tga") == 0 || path.extension().compare(".dds") == 0) {
			
			auto inserted = m_textures.insert({filename, std::unique_ptr<Texture>(Texture::Create(path.string()))});
			m_loadedTextures.push_back(filename);

			if (inserted.second) {
				// Queue upload to GPU

				std::unique_lock<std::mutex> lockFinished(m_finishedTexturesMutex);
				m_finishedTextures.push_back(inserted.first->second.get());
			}
		} else {
			SAIL_LOG_ERROR(filename + " does not have a supported texture format.");
		}
	} else {
		SAIL_LOG_ERROR("ResourceManager::loadTexture: Something is wrong with '" + filename + "'.");
	}
}
Texture& ResourceManager::getTexture(const std::string& filename) {
#ifdef USE_ONLY_THIS_TEXTURE
	*const_cast<std::string*>(&filename) = std::string(USE_ONLY_THIS_TEXTURE);
#endif
	auto pos = m_textures.find(filename);
	
	if (pos == m_textures.end())
		SAIL_LOG_ERROR("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().loadTexture(\"" + filename + "\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTexture(const std::string& filename) {
#ifdef USE_ONLY_THIS_TEXTURE
	*const_cast<std::string*>(&filename) = std::string(USE_ONLY_THIS_TEXTURE);
#endif
	return m_textures.find(filename) != m_textures.end();
}


//
// Model
//

// Used to add a model created from the application
void ResourceManager::addModel(const std::string& modelName, Model* model) {
	if (m_models.find(modelName) != m_models.end()) {
		SAIL_LOG_ERROR("The model name is already in use");
		return;
	}
	SAIL_LOG("Added model: " + modelName);
	model->setName(modelName);
	m_models.insert({modelName, std::unique_ptr<Model>(model)});
	m_byteSize[RMDataType::Models] = calculateModelByteSize();
}

bool ResourceManager::loadModel(const std::string& filename, Shader* shader, const ImporterType type) {
	// Insert the new model
	Shader* shaderToUse = shader ? shader : m_defaultShader;

	m_modelMutex.lock();
	if (m_models.find(filename) != m_models.end()) {
		m_modelMutex.unlock();
		return false;
	}
	m_modelMutex.unlock();
	Model* temp = nullptr;
	if (type == ResourceManager::ImporterType::SAIL_ASSIMP) {
		temp = m_assimpLoader->importModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shaderToUse);
	}
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		temp = m_fbxLoader->fetchModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shaderToUse);
	}

	if (temp) {
		SAIL_LOG("Loaded model: " + filename + " (" + std::to_string((float)temp->getByteSize() / (1024.f * 1024.f)) + "MB)");
		temp->setName(filename);
		m_modelMutex.lock();
		m_models.insert({ filename, std::unique_ptr<Model>(temp) });
		m_modelMutex.unlock();

		m_byteSize[RMDataType::Models] = calculateModelByteSize();

		return true;
	}
	else {
		SAIL_LOG_ERROR("Could not Load model: (" + filename + ")");

		m_byteSize[RMDataType::Models] = calculateModelByteSize();

		return false;
	}
}
Model& ResourceManager::getModel(const std::string& filename, Shader* shader, const ImporterType type) {
	auto pos = m_models.find(filename);
	if (pos == m_models.end()) {
		SAIL_LOG_WARNING("Tried to get model (" + filename + ") but it was not previously loaded.");
		// Model was not yet loaded, load it and return
		Shader* shaderToUse = shader ? shader : m_defaultShader;
		loadModel(filename, shaderToUse, type);
		
		return *m_models.find(filename)->second;
	}

	return *pos->second;
}
Model& ResourceManager::getModelCopy(const std::string& filename, Shader* shader) {
	Shader* shaderToUse = shader ? shader : m_defaultShader;
	Model& model = getModel(filename, shaderToUse);
	
	Mesh* mesh = model.getMesh(0);
	Mesh::Data data;
	data.deepCopy(mesh->getData());
	Model* tempModel = SAIL_NEW Model(data, shaderToUse);
	std::string nameCopy = getSuitableName(filename);
	tempModel->setName(nameCopy);
	SAIL_LOG("copied model: " + filename + ", using name: " + nameCopy);
	m_byteSize[RMDataType::Models] += tempModel->getByteSize();
	m_models.insert({ nameCopy, std::unique_ptr<Model>(tempModel) });

	return *m_models.find(nameCopy)->second;
}
bool ResourceManager::hasModel(const std::string& filename) {
	return m_models.find(filename) != m_models.end();
}

void ResourceManager::clearSceneData() {
	m_fbxLoader->clearAllScenes();
}

void ResourceManager::loadAnimationStack(const std::string& fileName, const ImporterType type) {
	AnimationStack* temp = nullptr;
	if (m_animationStacks.find(fileName) != m_animationStacks.end()) {
		return;
	}

	if (type == ResourceManager::ImporterType::SAIL_ASSIMP) {
		temp = m_assimpLoader->importAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName);
	}
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		//TODO: REMOVE SHADER FROM ANIMATION IMPORT
		assert(m_defaultShader&& "set default shader first, or load model first");
		temp = m_fbxLoader->fetchAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName, m_defaultShader);
	}

	if (temp) {
		m_animationStacks.insert({fileName, std::unique_ptr<AnimationStack>(temp)});
		SAIL_LOG("Animation size of '" + fileName + "' : " + std::to_string((float)m_animationStacks[fileName]->getByteSize() / (1024.f * 1024.f)) + "MB");
	}
	else {
		SAIL_LOG_ERROR("Could not Load model: (" + fileName + ")");
	}

	m_byteSize[RMDataType::Animations] = calculateAnimationByteSize();
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

FBXLoader* ResourceManager::getFBXLoader() const {
	return m_fbxLoader.get();
}

const unsigned int ResourceManager::numberOfModels() const {
	return m_models.size();
}

const unsigned int ResourceManager::numberOfTextures() const {
	return m_textures.size();
}

const unsigned int ResourceManager::getByteSize() const {
	unsigned int size = 0;
	for (int i = 0; i < 5; i++) {
		size += m_byteSize[i];
	}
	return size;
}

const unsigned int ResourceManager::getModelByteSize() const {
	return m_byteSize[RMDataType::Models];
}

const unsigned int ResourceManager::getAnimationsByteSize() const {
	return m_byteSize[RMDataType::Animations];
}

const unsigned int ResourceManager::getAudioByteSize() const {
	return m_byteSize[RMDataType::Audio];
}

const unsigned int ResourceManager::getTextureByteSize() const {
	return m_byteSize[RMDataType::Textures];
}

const unsigned int ResourceManager::getGenericByteSize() const {
	return m_byteSize[RMDataType::Generic];
}

void ResourceManager::uploadFinishedTextures(ID3D12GraphicsCommandList4* cmdList) {
	std::scoped_lock doubleLock(m_finishedTexturesMutex, m_textureDatasMutex);

	// Don't do anything if there are no textures to upload
	if (m_finishedTextures.empty()) {
		return;
	}

	// Upload and remove textures
	for (auto* tex : m_finishedTextures) {
		auto* dx12Tex = static_cast<DX12Texture*>(tex);
		dx12Tex->initBuffers(cmdList);
		
		const std::string& filename = dx12Tex->getFilename();
		
		// Remove tga data if it's a tga texture
		if (m_textureDatas.erase(filename) > 0) {
			continue;
		}

		// Remove dds data
		dx12Tex->clearDDSData();
	}
	m_finishedTextures.clear();

	// Reset map (very small amount of RAM)
	if (m_textureDatas.empty()) {
		m_textureDatas.clear();
		m_textureDatas = std::map<std::string, std::unique_ptr<TextureData>>();
	}

	m_byteSize[RMDataType::Textures] = calculateTextureByteSize();
}

void ResourceManager::clearModelCopies() {
	std::unique_lock lock(m_modelMutex);

	std::vector<std::string> modelsToRemove;

	// Find which models has a number after ".fbx"
	for (auto& model : m_models) {
		if (model.first.back() != 'x') {
			modelsToRemove.push_back(model.first);
		}
	}

	// Remove those models
	for (auto& modelToRemove : modelsToRemove) {
		m_models.erase(modelToRemove);
	}
}

#ifdef DEVELOPMENT
void ResourceManager::unloadTextures() {
	std::unique_lock<std::mutex> lockData(m_textureDatasMutex);
	m_textureDatas.clear();
	m_textureDatas = std::map<std::string, std::unique_ptr<TextureData>>();
	m_byteSize[RMDataType::Textures] = calculateTextureByteSize();
}

void ResourceManager::logRemainingTextures() const {
	std::unique_lock<std::mutex> lock(m_textureDatasMutex);
	
	for (auto& textureData : m_textureDatas) {
		SAIL_LOG(textureData.first + " still in CPU");
	}
}

void ResourceManager::printLoadedTexturesToFile() const {
	if (m_hasLoggedTextures) {
		return;
	}

	std::ofstream file;
	file.open("LoadedTextures.txt");
	
	for (auto& texture : m_loadedTextures) {
		file << texture + "\n";
	}

	file.close();

	m_hasLoggedTextures = true;
}

void ResourceManager::printModelsToFile() const {
	if (m_hasLoggedModels) {
		return;
	}

	std::ofstream file;
	file.open("LoadedModels.txt");

	std::unique_lock lock(m_modelMutex);
	for (auto& model : m_models) {
		file << model.first + "\n";
	}

	file.close();

	m_hasLoggedModels = true;
}
#endif

unsigned int ResourceManager::calculateTextureByteSize() const {
	// No lock needed since this is only called from this class,
	// and only from places where the lock has already been set
	unsigned int size = 0;
	for (auto& textureData : m_textureDatas) {
		size += textureData.second->getByteSize();
	}
	return size;
}

unsigned int ResourceManager::calculateAnimationByteSize() const {
	unsigned int size = 0;

	size += m_animationStacks.size() * sizeof(std::pair<std::string, std::unique_ptr<AnimationStack>>);

	for (auto& [key, val] : m_animationStacks) {
		size += key.capacity() * sizeof(unsigned char);
		size += val->getByteSize();
	}
	
	return size;
}

unsigned int ResourceManager::calculateModelByteSize() const {
	unsigned int size = 0;

	size += sizeof(std::pair<std::string, std::unique_ptr<Model>>) * m_models.size();

	for (auto& [key, val] : m_models) {
		size += key.capacity() * sizeof(unsigned char);
		size += val->getByteSize();
	}

	return size;
}

const std::string ResourceManager::getSuitableName(const std::string& name) {
	unsigned int iterator = 1;
	while (iterator < 1000) {
		std::string tempName = name + std::to_string(iterator++);
		if (m_models.find(tempName) == m_models.end()) {
			return tempName;
		}
	}
	return "broken";
}
