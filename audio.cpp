#include "audio.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <alsa/asoundlib.h>
#include <stdio.h>

#define PCM_DEVICE "default"

CAudio Audio;

void CAudio::Ring()
{
    if (!mbPlaying) {
        mbPlaying = true;
        thread ([this](){
            Thread();
        }).detach();
    }
}

#define CHECK(x) { if(!(x)) { \
fprintf(stderr, "%s:%i: failure at: %s\n", __FILE__, __LINE__, #x); \
_exit(1); } }

std::string freadStr(FILE* f, size_t len) {
    std::string s(len, '\0');
    CHECK(fread(&s[0], 1, len, f) == len);
    return s;
}

template<typename T>
T freadNum(FILE* f) {
    T value;
    CHECK(fread(&value, sizeof(value), 1, f) == 1);
    return value; // no endian-swap for now... WAV is LE anyway...
}

FILE* pFile;
int numChannels;
int sampleRate;
int bytesPerSample, bitsPerSample;

void readFmtChunk(uint32_t chunkLen) {
    CHECK(chunkLen >= 16);
    uint16_t fmttag = freadNum<uint16_t>(pFile); // 1: PCM (int). 3: IEEE float
    CHECK(fmttag == 1 || fmttag == 3);
    numChannels = freadNum<uint16_t>(pFile);
    CHECK(numChannels > 0);
    printf("%i channels\n", numChannels);
    sampleRate = freadNum<uint32_t>(pFile);
    printf("%i Hz\n", sampleRate);
    uint32_t byteRate = freadNum<uint32_t>(pFile);
    uint16_t blockAlign = freadNum<uint16_t>(pFile);
    bitsPerSample = freadNum<uint16_t>(pFile);
    bytesPerSample = bitsPerSample / 8;
    CHECK(byteRate == sampleRate * numChannels * bytesPerSample);
    CHECK(blockAlign == numChannels * bytesPerSample);
    if(fmttag == 1 /*PCM*/) {
        printf("PCM %ibit int\n", bitsPerSample);
    } else {
        CHECK(fmttag == 3 /* IEEE float */);
        CHECK(bitsPerSample == 32);
        printf("32bit float\n");
    }
    if(chunkLen > 16) {
        uint16_t extendedSize = freadNum<uint16_t>(pFile);
        CHECK(chunkLen == 18 + extendedSize);
        fseek(pFile, extendedSize, SEEK_CUR);
    }
}

void CAudio::Thread()
{
    unsigned int rate, channels, seconds;
    rate 	 = 44100;
    channels = 1;
    seconds  = 60;

    //
    pFile = fopen("c2c.wav", "rb");
    uint32_t chunkLen;
    CHECK(pFile != NULL);
    
    CHECK(freadStr(pFile, 4) == "RIFF");
    uint32_t wavechunksize = freadNum<uint32_t>(pFile);
    CHECK(freadStr(pFile, 4) == "WAVE");
    while(true) {
        std::string chunkName = freadStr(pFile, 4);
        chunkLen = freadNum<uint32_t>(pFile);
        if(chunkName == "fmt ")
            readFmtChunk(chunkLen);
        else if(chunkName == "data") {
            CHECK(sampleRate != 0);
            CHECK(numChannels > 0);
            CHECK(bytesPerSample > 0);
            printf("len: %.0f secs\n", double(chunkLen) / sampleRate / numChannels / bytesPerSample);
            break; // start playing now
        } else {
            // skip chunk
            CHECK(fseek(pFile, chunkLen, SEEK_CUR) == 0);
        }
    }

    rate 	 = sampleRate;
    channels = numChannels;
    seconds  = (chunkLen) / sampleRate / numChannels / bytesPerSample;

    // fseek(pFile, 44, SEEK_SET);
    //
    unsigned int pcm, tmp, dir;
    
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    char *buff;
    int buff_size, loops;

    /* Open the PCM device in playback mode */
    if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
                    SND_PCM_STREAM_PLAYBACK, 0) < 0) 
        printf("ERROR: Can't open \"%s\" PCM device. %s\n",
                    PCM_DEVICE, snd_strerror(pcm));

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(pcm_handle, params);

    /* Set parameters */
    if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
                    SND_PCM_ACCESS_RW_INTERLEAVED) < 0) 
        printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
                        SND_PCM_FORMAT_S16_LE) < 0) 
        printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0) 
        printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) 
        printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

    /* Write parameters */
    if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
        printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

    /* Resume information */
    printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

    snd_pcm_hw_params_get_channels(params, &tmp);
    printf("channels: %i ", tmp);

    if (tmp == 1)
        printf("(mono)\n");
    else if (tmp == 2)
        printf("(stereo)\n");

    snd_pcm_hw_params_get_rate(params, &tmp, 0);
    printf("rate: %d bps\n", tmp);

    printf("seconds: %d\n", seconds);	

    /* Allocate buffer to hold single period */
    snd_pcm_hw_params_get_period_size(params, &frames, 0);

    buff_size = frames * channels * 2 /* 2 -> sample size */;
    buff = (char *) malloc(buff_size);

    snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

    for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {

        if (pcm = fread(buff, 1, buff_size, pFile) == 0) {
            printf("Early end of file.\n");
            break;
        }

        if (pcm = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
            printf("XRUN.\n");
            snd_pcm_prepare(pcm_handle);
        } else if (pcm < 0) {
            printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
        }

    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    free(buff);
    fclose(pFile);
    mbPlaying = false;
}
