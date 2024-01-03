/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "sound.h"
#include "../simdebug.h"

#include <AL/al.h>
#include <AL/alc.h>

static ALCdevice *device;
static ALCcontext *context;

static ALuint buffers[1024];
static int next_buffer = 0;


bool dr_init_sound(void)
{
    device = alcOpenDevice(NULL);
    if (!device)
    {
        dbg->error("dr_init_sound()", "Could not open standard audio device. Error=%d", alGetError());
    	return false;
    }

    context = alcCreateContext(device, NULL);
    if (!alcMakeContextCurrent(context))
        dbg->error("dr_init_sound()", "Could not create audio context. Error=%d", alGetError());
    	return false;
    }

    ALuint source;


    alGenSources((ALuint)1, &source);
    // check for errors

    alSourcef(source, AL_PITCH, 1);
    // check for errors
    alSourcef(source, AL_GAIN, 1);
    // check for errors
    alSource3f(source, AL_POSITION, 0, 0, 0);
    // check for errors
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    // check for errors
    alSourcei(source, AL_LOOPING, AL_FALSE);

    
    
}



/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(const char *)
{
    alGenBuffers((ALuint)1, &buffers[next_buffer]);
    if((ALCenum error = alGetError()) != AL_NO_ERROR) {
        dbg->error("dr_init_sound()", "Could not create audio buffer. Error=%d", error);
    	return false;
    }
    
    ALsizei size, freq;
    ALenum format;
    ALvoid *data;
    ALboolean loop = AL_FALSE;

    alutLoadWAVFile("test.wav", &format, &data, &size, &freq, &loop);

    if((ALCenum error = alGetError()) != AL_NO_ERROR) {
        dbg->error("dr_init_sound()", "Could not read audio file. Error=%d", error);
    	return false;
    }

    
	return next_buffer ++;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int key, int volume)
{
    alSourcei(source, AL_BUFFER, buffers[key]);    
    alSourcePlay(source);
}



void dr_destroy_sound()
{
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);    
}
