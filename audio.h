#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <queue>

using namespace std;


#define RATE 44100  // Hz
#define FRAME_SIZE 2 // 16 bits
#define SAMPLE_SIZE 1024
#define FRAME_BY_SAMPLE (SAMPLE_SIZE / FRAME_SIZE)



struct CAudioSample {
    typedef shared_ptr<CAudioSample> Ptr;
    char buf [SAMPLE_SIZE];
};

class CAudio {
public :
    
    void Play ();
    void Push (CAudioSample::Ptr apSample);
    void Record ();
    void Stop ();
    
protected :
    void PlayThread ();
    thread mPlayThread;
    atomic<bool> mbPlay;
    void RecordThread ();
    thread mRecordThread;
    atomic<bool> mbRecord;
    mutex mMutexQueue;
    queue<CAudioSample::Ptr> mSamplesQueue;
};

extern CAudio Audio;
