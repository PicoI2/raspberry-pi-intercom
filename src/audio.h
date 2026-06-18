#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "speex/speex_echo.h"
#include <alsa/asoundlib.h>

using namespace std;

#define RATE 22050  // Hz
#define FRAME_SIZE 2 // 16 bits
#define SAMPLE_SIZE 4096
#define FRAME_BY_SAMPLE (SAMPLE_SIZE / FRAME_SIZE)  // Must be a power of 2, MAX 16384

struct CAudioSample {
    typedef shared_ptr<CAudioSample> Ptr;
    char buf [SAMPLE_SIZE];
};

class CAudio {
public :
    
    void Init ();
    void AudioOnOff (bool abOn);
    void Play ();
    void Push (CAudioSample::Ptr apSample);
    void Record ();
    void Stop ();
    bool SetOwner (string aOwner, bool bWantToOwn);
    string GetOwner () {return mOwner;};
    
protected :
    void StartThread ();
    void Thread ();
    bool OpenPcm (bool bRecord);    // Open and configure a PCM handle (worker thread only)
    void ClosePcm (bool bRecord);   // Drop and close a PCM handle (worker thread only)

    thread mThread;
    atomic<bool> mbThreadRunning {false};   // Is the worker thread alive
    atomic<bool> mbPlay;                     // Desired state: user wants playback
    atomic<bool> mbRecord;                   // Desired state: user wants recording
    mutex mMutexQueue;
    queue<CAudioSample::Ptr> mSamplesQueue;
    long mOutputAudioOn;
    atomic<char> mNbAudioUser;
    string mOwner;
    SpeexEchoState* mEchoState;
    // PCM handles are owned by the worker thread (open/close). Non-null means open.
    // mMutexPcm guards them so Stop() can safely snd_pcm_drop() to unblock the worker.
    mutex mMutexPcm;
    snd_pcm_t* mPlayPcmHandle;
    snd_pcm_t* mRecordPcmHandle;
};

extern CAudio Audio;
