#include "ring.h"
#include "config.h"
#include "audio.h"

#include <iostream>
#include <fstream>
#include <alsa/asoundlib.h>
#include <stdio.h>

CRing Ring;

void CRing::Init(boost::asio::io_service* apIoService)
{
    mpInterval = new boost::posix_time::millisec(30000); // 30 seconds
    mpTimer = new boost::asio::deadline_timer(*apIoService);
}

void CRing::Start()
{
    mpTimer->expires_from_now(*mpInterval);
    mpTimer->async_wait([this](const boost::system::error_code&){OnTimer();});
    if (!mbPlaying) {
        Audio.AudioOnOff(true);
        mbPlaying = true;
        mThread = thread ([this](){
            Thread();
        });
    }
}

void CRing::Thread()
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
        string chunkName = freadStr(4);
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
    unsigned int seconds = (chunkLen) / rate / channels / bytesPerSample;
    while (bOk && mbPlaying) {
        fseek(pFile, StartPos, SEEK_SET);
        unsigned int err;

        const char* name = Config.GetString("sound-card-play").c_str();
        if ('\0' == name[0]) {
            name = "default";
        }
        
        /* Open the PCM device in playback mode */
        snd_pcm_t *pcm_handle;
        if (err = snd_pcm_open(&pcm_handle, name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            cerr << "cannot open audio device " << name << "(" << snd_strerror (err) << ")" << endl;
        }

        /* Allocate parameters object and fill it with default values*/
        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);

        snd_pcm_hw_params_any(pcm_handle, params);

        /* Set parameters */
        if (err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
            cerr << "cannot set access type (" << snd_strerror (err) << ")" << endl;
        }

        if (err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE) < 0) {
            cerr << "cannot set sample format (" << snd_strerror (err) << ")" << endl;
        }

        if (err = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0) {
            cerr << "cannot set channel count (" << snd_strerror (err) << ")" << endl;
        }

        if (err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) {
            cerr << "cannot set sample rate (" << snd_strerror (err) << ")" << endl;
        }

        /* Write parameters */
        if (err = snd_pcm_hw_params(pcm_handle, params) < 0) {
            cerr << "cannot set parameters (" << snd_strerror (err) << ")" << endl;
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
            if (err = fread(buff, 1, buff_size, pFile) == 0) {
                cout << "Early end of file. " << endl;
                break;
            }
            if (err = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
                cout << "XRUN " << endl;
                snd_pcm_prepare(pcm_handle);
            } else if (err < 0) {
                cerr << "ERROR. Can't write to PCM device (" << snd_strerror(err) << ")" << endl;
            }
        }

        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
    }
    
    fclose(pFile);
    Audio.AudioOnOff(false);
    mbPlaying = false;
}

void CRing::OnTimer()
{
    Stop();
}

void CRing::Stop()
{
    mbPlaying = false;
    if (mThread.joinable()) {
        mThread.join();
    }
}