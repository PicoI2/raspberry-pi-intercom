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

    mEchoState = speex_echo_state_init(SAMPLE_BY_FRAME, FILTER_LENGTH);
    mPreprocessState = speex_preprocess_state_init(SAMPLE_BY_FRAME, RATE);
    int SampleRate = RATE;
    speex_echo_ctl(mEchoState, SPEEX_ECHO_SET_SAMPLING_RATE, &SampleRate);
    speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_ECHO_STATE, mEchoState);
    spx_int32_t ON = 1;
    speex_preprocess_ctl (mPreprocessState, SPEEX_PREPROCESS_SET_DENOISE, &ON);
    speex_preprocess_ctl (mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB, &ON);
    spx_int32_t EchoLevel = -40;
    speex_preprocess_ctl (mPreprocessState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &EchoLevel);
    spx_int32_t EchoLevelActive = -15;
    speex_preprocess_ctl (mPreprocessState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &EchoLevelActive);
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
        mPlayThread = thread ([this](){
            PlayThread();
        });
    }
}

// Push an audio frame in play queue
void CAudio::Push (CAudioFrame::Ptr apFrame) {
    if (mbPlay) {
        mMutexQueue.lock();
        // If more than 100 samples in queue (~4.6 seconds), delete the older one.
        if (mFramesQueue.size() > 100) {
            mFramesQueue.pop();
            cerr << "mFramesQueue is full" << endl;
        }
        mFramesQueue.push(apFrame);
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

    if (bRecord) {
        mRecordPcmHandle = PcmHandle;
    }
    else {
        mPlayPcmHandle = PcmHandle;
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

// Play thread
void CAudio::PlayThread()
{
    int err;

    CAudioFrame::Ptr pFrame;

    cout << "Start playing" << endl;

    while (mbPlay) {
        if (mFramesQueue.empty()) {
            // Wait for samples
            this_thread::sleep_for(chrono::milliseconds(1));
        }
        else {
            // Play sample
            mMutexQueue.lock();
            pFrame = mFramesQueue.front();
            mFramesQueue.pop();
            mMutexQueue.unlock();

            if (err = snd_pcm_writei(mPlayPcmHandle, pFrame->buf, SAMPLE_BY_FRAME) == -EPIPE) {
                cout << "XRUN " << endl;
                this_thread::sleep_for(chrono::milliseconds(80));
                snd_pcm_prepare(mPlayPcmHandle);
                snd_pcm_writei(mPlayPcmHandle, pFrame->buf, SAMPLE_BY_FRAME);
                speex_echo_playback (mEchoState, (spx_int16_t*)pFrame->buf);
            } else if (err < 0) {
                cerr << "ERROR. Can't write to PCM device (" << snd_strerror(err) << ")" << endl;
                speex_echo_state_reset (mEchoState);
                break;
            }
            else {
                speex_echo_playback (mEchoState, (spx_int16_t*)pFrame->buf);
            }
        }
    }

    cout << "Stop playing" << endl;
    StopPcm(false);
}

// Start record thread
void CAudio::Record()
{
    if (!mbRecord) {
        const char* name = Config.GetString("sound-card-rec").c_str();
        if ('\0' == name[0]) {
            name = "default";
        }
        StartPcm(name, true);
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
    cout << "Start recording" << endl;

    while (mbRecord) {
        CAudioFrame::Ptr pFrame (new CAudioFrame());
        if ((err = snd_pcm_readi (mRecordPcmHandle, pFrame->buf, SAMPLE_BY_FRAME)) != SAMPLE_BY_FRAME) {
            cerr << "read from audio interface failed (" << snd_strerror (err) << ")" << endl;
            break;
        }
        else if (mbPlay) {
            CAudioFrame::Ptr pFrameWithoutEcho (new CAudioFrame());
            speex_echo_capture (mEchoState, (spx_int16_t*)pFrame->buf, (spx_int16_t*)pFrameWithoutEcho->buf);
            speex_preprocess_run (mPreprocessState, (spx_int16_t*)pFrameWithoutEcho->buf);
            pFrame = pFrameWithoutEcho;
        }
        HttpServer.SendMessage(pFrame->buf, sizeof(pFrame->buf));
        Udp.Send(pFrame->buf, sizeof(pFrame->buf));
    }

    cout << "Stop recording" << endl;
    StopPcm(true);
}

// Stop all audio threads
void CAudio::Stop()
{
    cout << "Audio stop..." << endl;
    mbPlay = false;
    mbRecord = false;
    if (mRecordThread.joinable()) {
        mRecordThread.join();
    }
    if (mPlayThread.joinable()) {
        mPlayThread.join();
    }
    mMutexQueue.lock();
    while (!mFramesQueue.empty()) {
        mFramesQueue.pop();
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
