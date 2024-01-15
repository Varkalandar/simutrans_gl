/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <iostream>
#include <fstream>
#include <cstring>

#include "sound.h"
#include "../simdebug.h"

#include <AL/al.h>
#include <AL/alc.h>

static ALCdevice *device;
static ALCcontext *context;
static ALuint source;

static ALuint buffers[1024];
static int next_buffer = 0;


bool dr_init_sound(void)
{
    dbg->message("dr_init_sound()", "Open AL flavor init started ...");

    device = alcOpenDevice(NULL);
    if(!device) {
        dbg->error("dr_init_sound()", "Could not open standard audio device. Error=%d", alGetError());
    	return false;
    }

    context = alcCreateContext(device, NULL);
    bool ok = alcMakeContextCurrent(context);
    if(!ok) {
        dbg->error("dr_init_sound()", "Could not create audio context. Error=%d", alGetError());
    	return false;
    }

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

	return true;
}


std::int32_t convert_to_int(const char * buffer, std::size_t len)
{
    std::int32_t v = 0;
    
    if(len == 2) {
        v = (buffer[1] & 0xFF) << 8 | (buffer[0] & 0xFF);
    }
    else {
        v = (buffer[3] & 0xFF) << 24 | (buffer[2] & 0xFF) << 16 | (buffer[1] & 0xFF) << 8 | (buffer[0] & 0xFF);
    }
    
    dbg->message("convert_to_int", "%d", v);
    
    return v;
}


static bool load_wav_file_header(std::ifstream& file,
                          std::uint8_t& channels,
                          std::int32_t& sample_rate,
                          std::uint8_t& bits_per_sample,
                          ALsizei& size)
{
    char buffer[4];
    if(!file.is_open())
        return false;

    // the RIFF
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read RIFF");
        return false;
    }
    if(std::strncmp(buffer, "RIFF", 4) != 0)
    {
        dbg->error("load_wav_file_header", "file is not a valid WAVE file (header doesn't begin with RIFF)");
        return false;
    }

    // the size of the file
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read size of file");
        return false;
    }

    // the WAVE
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read WAVE");
        return false;
    }
    if(std::strncmp(buffer, "WAVE", 4) != 0)
    {
        dbg->error("load_wav_file_header", "file is not a valid WAVE file (header doesn't contain WAVE)");
        return false;
    }

    // "fmt/0"
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header"," could not read fmt/0");
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read the 16");
        return false;
    }

    // PCM should be 1?
    if(!file.read(buffer, 2))
    {
        dbg->error("load_wav_file_header", "could not read PCM");
        return false;
    }

    // the number of channels
    if(!file.read(buffer, 2))
    {
        dbg->error("load_wav_file_header", "could not read number of channels");
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read sample rate");
        return false;
    }
    sample_rate = convert_to_int(buffer, 4);

    // (sample_rate * bits_per_sample * channels) / 8
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read (sample_rate * bits_per_sample * channels) / 8");
        return false;
    }

    // ?? unknown
    if(!file.read(buffer, 2))
    {
        dbg->error("load_wav_file_header", "could not read 2 bytes");
        return false;
    }

    // bits_per_sample
    if(!file.read(buffer, 2))
    {
        dbg->error("load_wav_file_header", "could not read bits per sample");
        return false;
    }    
    bits_per_sample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read data chunk header");
        return false;
    }
    if(std::strncmp(buffer, "data", 4) != 0)
    {
        dbg->error("load_wav_file_header", "file is not a valid WAVE file (doesn't have 'data' tag)");
        return false;
    }

    // size of data
    if(!file.read(buffer, 4))
    {
        dbg->error("load_wav_file_header", "could not read data size");
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if(file.eof())
    {
        dbg->error("load_wav_file_header", "reached EOF on the file");
        return false;
    }
    if(file.fail())
    {
        dbg->error("load_wav_file_header", "fail state set on the file");
        return false;
    }

    dbg->message("load_wav_file_header", "size=%d channels=%d rate=%d bits=%d", size, channels, sample_rate, bits_per_sample);

    return true;
}


static char* load_wav(const char * filename,
               std::uint8_t& channels,
               std::int32_t& sample_rate,
               std::uint8_t& bits_per_sample,
               ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if(!in.is_open())
    {
        dbg->error("load_wav", "Could not open '%s'", filename);
        return nullptr;
    }
    if(!load_wav_file_header(in, channels, sample_rate, bits_per_sample, size))
    {
        dbg->error("load_wav", "Could not load wav header of '%s'", filename);
        return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}


/**
 * loads a sample
 * @return a handle for that sample or -1 on failure
 */
int dr_load_sample(const char * filename)
{
    dbg->message("dr_load_sample()", "Trying to load '%s'", filename);

    alGenBuffers((ALuint)1, &buffers[next_buffer]);
    ALCenum error;
    
    if((error = alGetError()) != AL_NO_ERROR) {
        dbg->error("dr_load_sample()", "Could not create audio buffer. Error=%d", error);
    	return false;
    }
    
    ALsizei size;
    std::uint8_t channels;
    std::int32_t sample_rate;
    std::uint8_t bits_per_sample;

    char * data = load_wav(filename, channels, sample_rate, bits_per_sample, size);
    
    if(data == nullptr) {
        dbg->error("dr_load_sample()", "Could not read audio file.");
    	return false;
    }

    ALenum format;
    if(channels == 1 && bits_per_sample == 8) {
        format = AL_FORMAT_MONO8;
    } 
    else if(channels == 1 && bits_per_sample == 16) {
        format = AL_FORMAT_MONO16;
    }
    else if(channels == 2 && bits_per_sample == 8) {
        format = AL_FORMAT_STEREO8;
    }
    else if(channels == 2 && bits_per_sample == 16) {
        format = AL_FORMAT_STEREO16;
    }
    else {
        dbg->error("dr_load_sample()", "unrecognised wave format channels=%d bps=%d", channels, bits_per_sample);
        return -1;
    }

    alBufferData(buffers[next_buffer], format, data, size, sample_rate);
    
	return next_buffer ++;
}


/**
 * plays a sample
 * @param key the key for the sample to be played
 */
void dr_play_sample(int key, int volume)
{
    dbg->message("dr_play_sample()", "playing sample %d at volume %d", key, volume);
    alSourcef(source, AL_GAIN, volume / 255.0 );
    alSourcei(source, AL_BUFFER, buffers[key]);    
    alSourcePlay(source);    
}



void dr_destroy_sound()
{
    alDeleteSources(1, &source);
    
    for(int i=0; i<next_buffer; i++) {
        alDeleteBuffers(1, &buffers[i]);
    }
    
    device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);    
}
