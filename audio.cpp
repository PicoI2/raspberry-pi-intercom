#include "audio.h"
#include "udp.h"
#include "httpserver.h"
#include "config.h"

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <iostream>

CAudio Audio;

// Start play thread
void CAudio::Play()
{
    if (!mbPlay) {
        mbPlay = true;
        mPlayThread = thread ([this](){
            PlayThread();
        });
    }
}

// Push an audio sample in play queue
void CAudio::Push (CAudioSample::Ptr apSample) {
    if (mbPlay) {
        mMutexQueue.lock();
        // If more than 100 samples in queue (~4.6 seconds), delete the older one.
        if (mSamplesQueue.size() > 100) {
            mSamplesQueue.pop();
            cerr << "mSamplesQueue is full" << endl;
        }
        mSamplesQueue.push(apSample);
        mMutexQueue.unlock();
    }
}

// Play thread
void CAudio::PlayThread()
{
    int err;
        
    /* Open the PCM device in playback mode */
    snd_pcm_t *pcm_handle;
    const char* name = Config.GetString("sound-card-play").c_str();
    if ('\0' == name[0]) {
        name = "default";
    }
    
    if (err = snd_pcm_open(&pcm_handle, name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        printf("ERROR: Can't open \"%s\" PCM device. %s\n", name, snd_strerror(err));
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(pcm_handle, params);

    /* Set parameters */
    if (err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
    }

    if (err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE) < 0) {
        printf("ERROR: Can't set format. %s\n", snd_strerror(err));
    }

    if (err = snd_pcm_hw_params_set_channels(pcm_handle, params, 1) < 0) {
        printf("ERROR: Can't set channels number. %s\n", snd_strerror(err));
    }
    unsigned int rate = RATE;
    if (err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) {
        printf("ERROR: Can't set rate. %s\n", snd_strerror(err));
    }

    cout << "snd_pcm_hw_params_set_rate_near to:" << rate << endl;

    /* Write parameters */
    if (err = snd_pcm_hw_params(pcm_handle, params) < 0)
        printf("ERROR: Can't set hardware parameters. %s\n", snd_strerror(err));

    CAudioSample::Ptr pSample;

    while (mbPlay) {
        while (mSamplesQueue.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        mMutexQueue.lock();
        pSample = mSamplesQueue.front();
        mSamplesQueue.pop();
        mMutexQueue.unlock();
        
        if (err = snd_pcm_writei(pcm_handle, pSample->buf, FRAME_BY_SAMPLE) == -EPIPE) {
            cout << "XRUN " << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            snd_pcm_prepare(pcm_handle);
        } else if (err < 0) {
            printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
            break;
        }
    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
}

// Start record thread
void CAudio::Record()
{
    if (!mbRecord) {
        mbRecord = true;
        mRecordThread = thread ([this](){
            RecordThread();
        });
    }
}

// Record thread
void CAudio::RecordThread()
{
    int err;
    snd_pcm_t *capture_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

    const char* name = Config.GetString("sound-card-rec").c_str();
    if ('\0' == name[0]) {
        name = "default";
    }
    if ((err = snd_pcm_open (&capture_handle, name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        cerr << "cannot open audio device " << name << "(" << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "audio interface opened" << endl;

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        cerr << "cannot allocate hardware parameter structure " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params allocated" << endl;

    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
        cerr << "cannot initialize hardware parameter structure " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params initialized" << endl;

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        cerr << "cannot set access type " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params access setted" << endl;

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
        cerr << "cannot set sample format " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params format setted" << endl;
    unsigned int rate = RATE;
    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
        cerr << "cannot set sample rate " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params rate setted to:" << rate << endl;

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
        cerr << "cannot set channel count " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params channels setted" << endl;

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
        cerr << "cannot set parameters " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "hw_params setted" << endl;

    snd_pcm_hw_params_free (hw_params);

    cout << "hw_params freed" << endl;

    if ((err = snd_pcm_prepare (capture_handle)) < 0) {
        cerr << "cannot prepare audio interface for use " << snd_strerror (err) << ")" << endl;
        return;
    }

    cout << "audio interface prepared" << endl;
    cout << " snd_pcm_format_width(format) " <<  snd_pcm_format_width(format) << "bits" << endl;

    while (mbRecord) {
        CAudioSample Sample;
        if ((err = snd_pcm_readi (capture_handle, Sample.buf, FRAME_BY_SAMPLE)) != FRAME_BY_SAMPLE) {
            cerr << "read from audio interface failed " << snd_strerror (err) << ")" << endl;
            break;
        }
        HttpServer.SendMessage(Sample.buf, sizeof(Sample.buf));
        Udp.Send(Sample.buf, sizeof(Sample.buf));
    }

    snd_pcm_close (capture_handle);
    cout << "audio interface closed" << endl;
}

// Stop all audio threads
void CAudio::Stop()
{
    mbPlay = false;
    mbRecord = false;
    if (mRecordThread.joinable()) {
        mRecordThread.join();
    }
    if (mPlayThread.joinable()) {
        mPlayThread.join();
    }
    mMutexQueue.lock();
    while (!mSamplesQueue.empty()) {
        mSamplesQueue.pop();
    }
    mMutexQueue.unlock();
}