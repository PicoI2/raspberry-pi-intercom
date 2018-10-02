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

void CAudio::Thread()
{
    FILE* pFile = fopen("ring.wav", "rb");
    bool bOk = (pFile != NULL);

    // Locals lambda function
    auto fread16 = [&]() {
        uint16_t value;
        bOk = bOk && (fread(&value, sizeof(value), 1, pFile) == 1);
        return value;
    };
    auto fread32 = [&]() {
        uint32_t value;
        bOk = bOk && (fread(&value, sizeof(value), 1, pFile) == 1);
        return value;
    };
    auto freadStr = [&] (size_t len) {
        string s(len, '\0');
        bOk = bOk && (fread(&s[0], 1, len, pFile) == len);
        return s;
    };

    // Read WAV headers
    unsigned int rate, channels;
    int bytesPerSample, bitsPerSample;
    
    uint32_t chunkLen;
    bOk = bOk && (freadStr(4) == "RIFF");
    uint32_t wavechunksize = fread32();
    bOk = bOk && (freadStr(4) == "WAVE");
    while (bOk) {
        std::string chunkName = freadStr(4);
        chunkLen = fread32();
        if (chunkName == "fmt ") {
            bOk = bOk && (chunkLen >= 16);
            uint16_t fmttag = fread16(); // 1: PCM (int). 3: IEEE float
            bOk = bOk && (fmttag == 1 || fmttag == 3);
            channels = fread16();
            bOk = bOk && (channels > 0);
            rate = fread32();
            uint32_t byteRate = fread32();
            uint16_t blockAlign = fread16();
            bitsPerSample = fread16();
            bytesPerSample = bitsPerSample / 8;
            bOk = bOk && (byteRate == rate * channels * bytesPerSample);
            bOk = bOk && (blockAlign == channels * bytesPerSample);
            bOk = bOk && ((fmttag == 1) || (fmttag == 3 && bitsPerSample == 32));
            if (chunkLen > 16) {
                uint16_t extendedSize = fread16();
                bOk = bOk && (chunkLen == 18 + extendedSize);
                fseek(pFile, extendedSize, SEEK_CUR);
            }
        }
        else if (chunkName == "data") {
            bOk = bOk && (rate > 0 && channels > 0 && bytesPerSample > 0);
            break; // start playing now
        } else {
            // skip chunk
            bOk = bOk && (fseek(pFile, chunkLen, SEEK_CUR) == 0);
        }
    }

    size_t StartPos = ftell (pFile);
    while (bOk && mbPlaying) {
        fseek(pFile, StartPos, SEEK_SET);
        unsigned int pcm;
        
        /* Open the PCM device in playback mode */
        snd_pcm_t *pcm_handle;
        if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            printf("ERROR: Can't open \"%s\" PCM device. %s\n", PCM_DEVICE, snd_strerror(pcm));
        }

        /* Allocate parameters object and fill it with default values*/
        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);

        snd_pcm_hw_params_any(pcm_handle, params);

        /* Set parameters */
        if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
            printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
        }

        if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE) < 0) {
            printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));
        }

        if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0) {
            printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));
        }

        if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) {
            printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));
        }

        /* Write parameters */
        if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
            printf("ERROR: Can't set hardware parameters. %s\n", snd_strerror(pcm));{
        }

        /* Allocate buffer to hold single period */
        snd_pcm_uframes_t frames;
        snd_pcm_hw_params_get_period_size(params, &frames, 0);

        size_t buff_size = frames * channels * 2 /* 2 -> sample size */;
        char buff [buff_size];
        unsigned int period;
        snd_pcm_hw_params_get_period_time(params, &period, NULL);

        unsigned int seconds  = (chunkLen) / rate / channels / bytesPerSample;
        for (unsigned int loops = (seconds * 1000000) / period; loops > 0 && mbPlaying; loops--) {
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
    }
    
    fclose(pFile);
    mbPlaying = false;
}

void CAudio::Stop()
{
    mbPlaying = false;
}