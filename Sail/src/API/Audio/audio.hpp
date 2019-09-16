#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Xaudio2.h"
#include <thread>

#define SOUND_COUNT 3
#define STREAMING_BUFFER_SIZE 65536
#define MAX_BUFFER_COUNT 3

class Audio
{
public:
	Audio();
	~Audio();

	void loadSound(const std::string &filename);
	int playSound(const std::string& filename);
	int streamSound(const std::string& filename);
	void pauseSound(int index);
	void pauseAllSounds();

	void updateAudio();

private:

	// TEMPORARY *-*-*-*-*-*-*-*-//
	bool m_singlePress1 = true; //
	bool m_singlePress2 = true;//
	// *-*-*-*-*-*-*-*-*-*-*-*//

	// Main 'interface' object managing all audio engine states
	IXAudio2* m_xAudio2 = nullptr;
	// Represents the audio output device
	IXAudio2MasteringVoice* m_masterVoice = nullptr;
	// Represents each loaded sound in the form of an 'object'
	IXAudio2SourceVoice* m_sourceVoice[SOUND_COUNT];
	
	int m_currIndex = 0;

	BYTE m_streamBuffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
	OVERLAPPED m_overlapped = { 0 };
	std::thread* m_tempStreamThread = nullptr;

	// PRIVATE FUNCTION
	//-----------------
	void initXAudio2();
	void streamingLoop(const std::string& filename, IXAudio2SourceVoice* streamedSourceVoice);
	std::wstring s2ws(const std::string& s);
	// ----------------
};

#endif
