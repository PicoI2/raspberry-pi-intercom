#include "audio.h"
#include "udp.h"
#include "httpserver.h"
#include "config.h"
#include "io.h"

#include <stdio.h>
#include <iostream>

CAudio Audio;

// Initialization
void CAudio::Init()
{
    mNbAudioUser = 0;
    mOutputAudioOn = Config.GetULong("output-audio-on", false);
    if (mOutputAudioOn) {
        IO.AddOutput(mOutputAudioOn, false);
    }

    mEchoState = speex_echo_state_init(FRAME_SIZE, SAMPLE_SIZE);
}

// Set audio output on/off
void CAudio::AudioOnOff (bool abOn)
{
    if (mOutputAudioOn) {
        if (abOn) {
            ++mNbAudioUser;
        }
        else {
            --mNbAudioUser;
        }
        assert (mNbAudioUser >= 0 && mNbAudioUser <= 2);
        IO.SetOutput(mOutputAudioOn, mNbAudioUser > 0);
    }
}

// Start play thread
void CAudio::Play()
{
    if (!mbPlay) {
        const char* name = Config.GetString("sound-card-play").c_str();
        if ('\0' == name[0]) {
            name = "default";
        }
        StartPcm(name, false);
        mbPlay = true;
        AudioOnOff(true);
        StartThread();
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

int CAudio::StartPcm(const char* aName, bool bRecord)
{
    int err;
        
    /* Open the PCM device in playback mode */
    snd_pcm_t* PcmHandle;
    if (err = snd_pcm_open(&PcmHandle, aName, bRecord ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        cerr << "ERROR: Can't open " << aName << " PCM device (" << snd_strerror (err) << ")" << endl;
    }
    if (bRecord) {
        mRecordPcmHandle = PcmHandle;
    }
    else {
        mPlayPcmHandle = PcmHandle;
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(PcmHandle, params);

    /* Set parameters */
    if (err = snd_pcm_hw_params_set_access(PcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        cerr << "ERROR: Can't set interleaved mode (" << snd_strerror (err) << ")" << endl;
    }

    if (err = snd_pcm_hw_params_set_format(PcmHandle, params, SND_PCM_FORMAT_S16_LE) < 0) {
        cerr << "ERROR: Can't set format (" << snd_strerror (err) << ")" << endl;
    }

    if (err = snd_pcm_hw_params_set_channels(PcmHandle, params, 1) < 0) {
        cerr << "ERROR: Can't set channels number (" << snd_strerror (err) << ")" << endl;
    }
    unsigned int rate = RATE;
    if (err = snd_pcm_hw_params_set_rate_near(PcmHandle, params, &rate, 0) < 0) {
        cerr << "ERROR: Can't set rate (" << snd_strerror (err) << ")" << endl;
    }

    cout << "snd_pcm_hw_params_set_rate_near to:" << rate << endl;

    /* Write parameters */
    if (err = snd_pcm_hw_params(PcmHandle, params) < 0) {
        cerr << "ERROR: Can't set hardware parameters (" << snd_strerror (err) << ")" << endl;
    }
    return err;
}

void CAudio::StopPcm(bool bRecord)
{
    snd_pcm_t* PcmHandle = bRecord ? mRecordPcmHandle : mPlayPcmHandle;
    cout << "Stop playing" << endl;

    snd_pcm_drain(PcmHandle);
    snd_pcm_close(PcmHandle);

    if (!bRecord) {
        AudioOnOff(false);
    }
}

void CAudio::StartThread() {
    if (!mThread.joinable()) {
        mThread = thread ([this](){
            Thread();
        });
    }
}

// Play and record thread
void CAudio::Thread()
{
    int err;
    CAudioSample::Ptr pPlaySample;

    cout << "Start Thread" << endl;

    while (mbPlay || mbRecord) {
        if (mbPlay) {
            if (mSamplesQueue.empty() && !mbRecord) {
                // Wait for samples
                this_thread::sleep_for(chrono::milliseconds(1));
            }
            else if (!mSamplesQueue.empty()) {
                // Play sample
                mMutexQueue.lock();
                pPlaySample = mSamplesQueue.front();
                mSamplesQueue.pop();
                mMutexQueue.unlock();
                
                if (err = snd_pcm_writei(mPlayPcmHandle, pPlaySample->buf, FRAME_BY_SAMPLE) == -EPIPE) {
                    cout << "XRUN " << endl;
                    this_thread::sleep_for(chrono::milliseconds(80));
                    snd_pcm_prepare(mPlayPcmHandle);
                } else if (err < 0) {
                    cerr << "ERROR. Can't write to PCM device (" << snd_strerror(err) << ")" << endl;
                    break;
                }
            }
        }
        
        if (mbRecord) {
            CAudioSample::Ptr pSample (new CAudioSample());
            if ((err = snd_pcm_readi (mRecordPcmHandle, pSample->buf, FRAME_BY_SAMPLE)) != FRAME_BY_SAMPLE) {
                cerr << "read from audio interface failed (" << snd_strerror (err) << ")" << endl;
                break;
            }
            if (pPlaySample) {
                CAudioSample::Ptr pSampleWithoutEcho (new CAudioSample());
                speex_echo_cancellation(mEchoState, (spx_int16_t*)pSample->buf, (spx_int16_t*)pPlaySample->buf, (spx_int16_t*)pSampleWithoutEcho->buf);
                pSample = pSampleWithoutEcho;
            }
            
            HttpServer.SendMessage(pSample->buf, SAMPLE_SIZE);
            Udp.Send(pSample->buf, SAMPLE_SIZE);
        }
    }
}

// Start recording
void CAudio::Record()
{
    if (!mbRecord) {
        const char* name = Config.GetString("sound-card-rec").c_str();
        if ('\0' == name[0]) {
            name = "default";
        }
        StartPcm(name, true);
        mbRecord = true;
        StartThread();
    }
}

// Stop audio thread
void CAudio::Stop()
{
    cout << "Audio stop..." << endl;
    bool bWasPlaying = mbPlay;
    bool bWasRecording = mbRecord;
    mbPlay = false;
    mbRecord = false;
    if (mThread.joinable()) {
        mThread.join();
    }
    if (bWasPlaying) {
        StopPcm(false);
    }
    if (bWasRecording) {
        StopPcm(true);
    }
    
    mMutexQueue.lock();
    while (!mSamplesQueue.empty()) {
        mSamplesQueue.pop();
    }
    mMutexQueue.unlock();
    cout << "Audio stopped" << endl;
}

// Set owner for audio from distant to local speaker.
// This is to manage case where 2 user answer in the same time
// Return true if succeed, false if an user owns already audio canal
bool CAudio::SetOwner (string aOwner, bool bWantToOwn)
{
    if (mOwner.empty() && bWantToOwn) {
        mOwner = aOwner;
        HttpServer.SendMessage("audiobusy");
    }
    bool bRes = (aOwner == mOwner);
    if (bRes && !bWantToOwn) {
        mOwner.clear();
        HttpServer.SendMessage("audiofree");
    }
    return bRes;
}
