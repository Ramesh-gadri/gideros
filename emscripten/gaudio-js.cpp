#include <gaudio.h>
#include "ggaudiomanager.h"

#if defined(OPENAL_SUBDIR_OPENAL)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#elif defined(OPENAL_SUBDIR_AL)
#include <AL/al.h>
#include <AL/alext.h>
#include <AL/alc.h>
#else
#include <al.h>
#include <alc.h>
#endif

extern "C" {
GGSampleInterface *GGSampleOpenALManagerCreate();
void GGSampleOpenALManagerDelete(GGSampleInterface *manager);

GGStreamInterface *GGStreamOpenALManagerCreate();
void GGStreamOpenALManagerDelete(GGStreamInterface *manager);
}

struct GGAudioSystemData
{
    ALCdevice *device;
    ALCcontext *context;
};

void GGAudioManager::systemInit()
{
    systemData_ = (GGAudioSystemData*)malloc(sizeof(GGAudioSystemData));

    systemData_->device = alcOpenDevice(NULL);

    systemData_->context = alcCreateContext(systemData_->device, NULL);

    alcMakeContextCurrent(systemData_->context);

}

void GGAudioManager::systemCleanup()
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(systemData_->context);
    alcCloseDevice(systemData_->device);

    free(systemData_);
}

void GGAudioManager::createBackgroundMusicInterface()
{
    backgroundMusicInterface_ = NULL;
}

void GGAudioManager::deleteBackgroundMusicInterface()
{
}

void GGAudioManager::beginInterruption()
{
}

void GGAudioManager::endInterruption()
{
}

void GGSoundManager::interfacesInit()
{
    loaders_["wav"] = GGAudioLoader(gaudio_WavOpen, gaudio_WavClose, gaudio_WavRead, gaudio_WavSeek, gaudio_WavTell);

    sampleInterface_ = GGSampleOpenALManagerCreate();
    streamInterface_ = GGStreamOpenALManagerCreate();
}

void GGSoundManager::interfacesCleanup()
{
    GGSampleOpenALManagerDelete(sampleInterface_);
    GGStreamOpenALManagerDelete(streamInterface_);
}

void GGAudioManager::AdvanceStreamBuffers()
{
    soundManager_->AdvanceStreamBuffers();
	alcDeviceTickSOFT(systemData_->device);
}

void GGSoundManager::AdvanceStreamBuffers()
{
    streamInterface_->AdvanceStreamBuffers();
}
