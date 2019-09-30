#include "pch.h"

#include "AudioSystem.h"
#include "..//Sail/src/Sail/entities/components/AudioComponent.h"
#include "..//..//Entity.h"
#include <iterator>
#include "..//Sail/src/Sail/Application.h"

AudioSystem::AudioSystem() {
	requiredComponentTypes.push_back(AudioComponent::ID);
	m_audioEngine.loadSound("../Audio/footsteps_1.wav");
	m_audioEngine.loadSound("../Audio/jump.wav");
}

AudioSystem::~AudioSystem() {
	m_audioEngine.stopAllSounds();
}

void AudioSystem::update(float dt) {

	AudioComponent* audioC = nullptr;

	for (auto& e : entities) {

		audioC = e->getComponent<AudioComponent>();

		if (audioC != nullptr) {

			// Playing NORMAL sounds
			for (int i = 0; i < SoundType::COUNT; i++) {

				if (audioC->m_isPlaying[i]) {
					if (audioC->m_soundEffectTimers[i] == 0.0f) {
						audioC->m_soundID[i] = m_audioEngine.playSound(audioC->m_soundEffects[i]);
						audioC->m_soundEffectTimers[i] += dt;

						audioC->m_isPlaying[i] = !audioC->m_playOnce[i]; // NOTE: Opposite, hence '!'
					}

					else {
						audioC->m_soundEffectTimers[i] += dt;

						if (audioC->m_soundEffectTimers[i] > audioC->m_soundEffectThresholds[i]) {
							audioC->m_soundEffectTimers[i] = 0.0f;
						}
					}
				}

				else if (audioC->m_soundEffectTimers[i] != 0.0f && !audioC->m_playOnce[i]) {
					m_audioEngine.stopSpecificSound(audioC->m_soundID[i]);
					audioC->m_soundEffectTimers[i] = 0.0f;
				}

				// NOTE:
				//		This is a correct solution for FADING, I believe, however it has to be done using a thread
				//
				//else if (audioC->m_soundEffectTimers[i] != 0.0f && !audioC->m_playOnce[i]) {
					//float volumeHolder = m_audioEngine.getSoundVolume(audioC->m_soundID[i]);
					//if (volumeHolder > 0.0f) {
					//	m_audioEngine.setSoundVolume(audioC->m_soundID[i], (volumeHolder - (1.0f * dt)));
					//}
					//else {
					//	m_audioEngine.stopSpecificSound(audioC->m_soundID[i]);
					//	audioC->m_soundEffectTimers[i] = 0.0f;
					//}
				//}
			}
		}

			// Playing STREAMED sounds
		std::list<std::pair<std::string, bool>>::iterator i;
		std::list<std::pair<std::string, bool>>::iterator toBeDeleted;
		std::list<std::pair<std::string, int>>::iterator j;
		std::list<std::pair<std::string, int>>::iterator streamToBeDeleted;
		std::string filename = "";
		int IDHolder = 0;

		for (i = audioC->m_streamingRequests.begin(); i != audioC->m_streamingRequests.end();) {

			if (i->second == true) {

				filename = i->first;
				toBeDeleted = i;
				i++;

				//if (audioC->m_currentlyStreaming.find(filename) != audioC->m_currentlyStreaming.end()) {
				//	std::cout << "OH SHIET\n";
				//}
				IDHolder = m_audioEngine.getStreamIndex();

				Application::getInstance()->pushJobToThreadPool(
					[this, filename](int id) {
						return m_audioEngine.streamSound(filename);
					});

				audioC->m_currentlyStreaming.emplace_back(filename, IDHolder);
				audioC->m_streamingRequests.erase(toBeDeleted);
				// NOTE: We are NOT incrementing because we JUST ERASED an element from the map
			}
			else/*if (i.second == false)*/ {

				filename = i->first;
				toBeDeleted = i;
				i++;

				for (j = audioC->m_currentlyStreaming.begin(); j != audioC->m_currentlyStreaming.end();) {

					streamToBeDeleted = j;
					j++;

					if (streamToBeDeleted->first == filename) {

						bool expectedValue = true;
						while (m_audioEngine.m_streamLocks[streamToBeDeleted->second].compare_exchange_strong(expectedValue, true));

						m_audioEngine.stopSpecificStream(streamToBeDeleted->second);
						m_audioEngine.m_streamLocks[streamToBeDeleted->second].store(false);
						audioC->m_currentlyStreaming.erase(streamToBeDeleted);

						break;
					}
				}
				audioC->m_streamingRequests.erase(toBeDeleted);
			}
		}
	}
}

void AudioSystem::stop() {
	m_audioEngine.stopAllStreams();
}

AudioEngine* AudioSystem::getAudioEngine() {
	return &m_audioEngine;
}